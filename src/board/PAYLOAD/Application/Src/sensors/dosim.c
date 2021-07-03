
#include "stm32f4xx_hal.h"

#include "sensors/dosim.h"

#include "util.h"
#include "time_svc.h"


//! Длина интервала накопления тиков (мс)
#define DOSIM_MEASURE_PERIOD (1*1000)

//! На сколькуо МС будем аккамулировать данные (мс)
/*! В интернете попалась цифра в 75 секунд экспозиции */
#define DOSIM_MEASURE_WINDOW (75*1000)

//! Количество мелких окон, которые мы храним
#define DOSIM_MEASURES (100)


extern  TIM_HandleTypeDef htim3;

//! Одно измерение дозиметра в каком-то окне
typedef struct dosim_measure_t
{
	//! Начало интервала накопления измерений
	uint32_t interval_begin;
	//! Конец интервала накопления измерений
	uint32_t interval_end;

	// Казалось бы нам не нужно время интервалов
	// но однако на случай, если где-то такты будут затягиваться
	// мы его вставим

	//! Количество накопленных тиков за этот интервал
	uint32_t accumulated_tics;

} dosim_measure_t;


//! Набор измерений дозиметра за несколько периодов
typedef struct dosim_measure_buf_t
{
	//! Указатель на подлежащий линейный буфер
	dosim_measure_t * linear_buffer;
	//! размер линейного буфера
	size_t capacity;
	//! Голова циклобуфера
	size_t head;
	//! Размер циклобуфера
	size_t size;

	//! Мировое время последнего коммита в буфер
	struct timeval last_commit_time;
	uint32_t last_commit_time_steady;
} dosim_measure_buf_t;


//! Инициализация циклобуфера
static int _measure_buf_init(dosim_measure_buf_t * buf, dosim_measure_t * linear_buf, size_t linear_buf_size)
{
	buf->linear_buffer = linear_buf;
	buf->capacity = linear_buf_size;
	buf->head = 0;
	buf->size = 0;

	memset(buf->linear_buffer, 0x00, sizeof(dosim_measure_t) * buf->capacity);

	buf->linear_buffer[buf->head].interval_begin = HAL_GetTick();
	buf->linear_buffer[buf->head].accumulated_tics = 0;
	buf->linear_buffer[buf->head].interval_end = 0;

	return 0;
}


//! Фиксируем голову и переходим к следующему элементу
static void _measure_buf_commit(dosim_measure_buf_t * buf, uint32_t ticks)
{
	buf->linear_buffer[buf->head].accumulated_tics = ticks;
	buf->linear_buffer[buf->head].interval_end = HAL_GetTick();

	buf->head += 1;
	if (buf->head >= buf->capacity)
		buf->head = 0;

	if (buf->size < buf->capacity)
		buf->size += 1;

	buf->linear_buffer[buf->head].interval_begin = HAL_GetTick();
	buf->linear_buffer[buf->head].accumulated_tics = 0;
	buf->linear_buffer[buf->head].interval_end = 0;

	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	buf->last_commit_time = tmv;
	buf->last_commit_time_steady = HAL_GetTick();
}


//! Сумма тиков на указанную длину по времени
/*! Передаваемый range - какое расстроение в мс мы хотим пройти
 *  после вызова функции, туда запишется сколько в мс мы реально прошли */
static void _measure_buf_sum_for_range(dosim_measure_buf_t * buf, uint32_t * range_, int64_t * tics_)
{
	if (0 == buf->size)
	{
		*range_ = 0;
		*tics_ = 0;
		return;
	}

	// Идем от головы назад
	size_t walker = buf->head;
	// Голова указывает на следующий элемент после существующего
	if (walker > 0)
		walker -= 1;
	else
		walker = buf->capacity - 1;

	uint64_t tics = 0;
	size_t distance = 0;
	uint32_t range = 0;
	const uint32_t requested_range = *range_;
	const uint32_t head_interval_end = buf->linear_buffer[walker].interval_end;
	while(1)
	{
		// Если прошли весь буфер
		if (distance > buf->size)
			break;

		if (range > requested_range)
			break;

		const dosim_measure_t * const m = &buf->linear_buffer[walker];
		tics += m->accumulated_tics;
		distance += 1;
		range = head_interval_end - m->interval_begin;

		if (walker > 0)
			walker -= 1;
		else
			walker = buf->capacity - 1;
	}

	*tics_ = tics;
	*range_ = range;
	return;
}


typedef struct dosim_t {
	TIM_HandleTypeDef * dosim_timer;
	dosim_measure_t _measure_linear_buff[DOSIM_MEASURES];
	dosim_measure_buf_t measure_buf;
	uint32_t measure_period;
} dosim_t;


static dosim_t _dosim;



void dosim_init()
{
	dosim_t * const self = &_dosim;
	dosim_measure_buf_t * const buf = &self->measure_buf;

	_measure_buf_init(buf, self->_measure_linear_buff, DOSIM_MEASURES);
	self->measure_period = DOSIM_MEASURE_PERIOD;

	self->dosim_timer = &htim3;
	self->dosim_timer->Instance->CNT = 0;
	HAL_TIM_Base_Start(self->dosim_timer);
}


void dosim_work()
{
	dosim_t * const self = &_dosim;
	dosim_measure_buf_t * const buf = &self->measure_buf;
	dosim_measure_t * const head = &buf->linear_buffer[buf->head];

	const uint32_t now = HAL_GetTick();
	if (now - head->interval_begin > self->measure_period)
	{
		uint32_t ticks = __HAL_TIM_GET_COUNTER(self->dosim_timer);
		__HAL_TIM_SET_COUNTER(self->dosim_timer, 0);
		_measure_buf_commit(buf, ticks);
	}
}


void dosim_read_momentary(mavlink_pld_dosim_data_t * msg)
{
	dosim_t * const self = &_dosim;
	dosim_measure_buf_t * const buf = &self->measure_buf;

	if (0 == buf->size)
	{
		// Нету у нас еще ничего
		struct timeval tmv;
		time_svc_gettimeofday(&tmv);
		msg->time_s = tmv.tv_sec;
		msg->time_us = tmv.tv_usec;
		msg->time_steady = HAL_GetTick();

		msg->count_tick = 0;
		msg->delta_time = 0;
		return;
	}

	size_t prehead_index;
	if (buf->head > 0)
		prehead_index = buf->head - 1;
	else
		prehead_index = buf->capacity - 1;


	msg->time_s = buf->last_commit_time.tv_sec;
	msg->time_us = buf->last_commit_time.tv_usec;
	msg->time_steady = buf->last_commit_time_steady;

	const dosim_measure_t * const pre_head = &buf->linear_buffer[prehead_index];
	msg->count_tick = pre_head->accumulated_tics;
	msg->delta_time = pre_head->interval_end - pre_head->interval_begin;

}


void dosim_read_windowed(mavlink_pld_dosim_data_t * msg)
{
	dosim_t * const self = &_dosim;
	dosim_measure_buf_t * const buf = &self->measure_buf;

	if (0 == buf->size)
	{
		// Нету у нас еще ничего
		struct timeval tmv;
		time_svc_gettimeofday(&tmv);
		msg->time_s = tmv.tv_sec;
		msg->time_us = tmv.tv_usec;
		msg->time_steady = HAL_GetTick();

		msg->count_tick = 0;
		msg->delta_time = 0;
		return;
	}

	uint32_t range = DOSIM_MEASURE_WINDOW;
	int64_t tics;
	_measure_buf_sum_for_range(buf, &range, &tics);

	msg->time_s = buf->last_commit_time.tv_sec;
	msg->time_us = buf->last_commit_time.tv_usec;
	msg->time_steady = buf->last_commit_time_steady;

	msg->count_tick = tics;
	msg->delta_time = range;
}
