/*
 * time_svc.c
 *
 *  Created on: Jun 12, 2020
 *      Author: snork
 */


#include "time_svc.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <inttypes.h>
#include <stdio.h>


// Целые части секунд
static time_t _seconds;


// Запоминаем метку времени в которое мы получили последний
// синхроимпульс
// используем двойную буферизацию для борьбы с внезваными прерываниями
static struct timeval _last_sync_tmv_1;
static struct timeval _last_sync_tmv_2;
static struct timeval * _last_sync_tmv_inner = &_last_sync_tmv_1;
static struct timeval * _last_sync_tmv_outer = &_last_sync_tmv_2;
// Откуда мы взяли время
static uint8_t _my_timebase = TIME_BASE_TYPE_NONE;
// Количество раз сколько мы пытались синхронизовать время
static uint16_t _time_sync_attempts = 0;
// Количество раз сколько мы синхронизовали время
static uint16_t _time_syncs_count = 0;
// Время последней коррекции (по шкале HAL_GetTick())
static uint32_t _last_sync_ts = 0;
// Величина последней коррекции
static int64_t _last_sync_delta_s = 0;
static int64_t _last_sync_delta_us = 0;

// Ожидается ли
static bool _sync_pending = false;

// Проверка - есть ли ожидающий свапа таймштамп
static bool _check_swap_sync()
{
	if (!_sync_pending)
		return false;

	__disable_irq();
	struct timeval * tmp = _last_sync_tmv_inner;
	_last_sync_tmv_inner = _last_sync_tmv_outer;
	_last_sync_tmv_outer = tmp;
	_sync_pending = false;
	__enable_irq();

	return true;
}


static void _tmv_normalize(const struct timeval * value)
{
	// Обрабатываем переполнения микросекунд
	if (value->tv_usec > 1000*1000)
	{
		int64_t dv = value->tv_usec / (1000*1000);
		value->tv_sec += dv;
		value->tv_usec -= dv*1000*1000;
	}

	if (value->tv_usec < -1*1000*1000)
	{
		int64_t dv = value->tv_usec / (-1*1000*1000);
		value->tv_sec -= dv;
		value->tv_usec += dv * 1000*1000;
	}

	// Если микросекунды меньше нуля, но секунды при этом есть - перекидываем знак в секунды
	if (value->tv_sec != 0 && value->tv_usec < 0)
	{
		value->tv_sec -= 1;
		value->tv_usec += 1000*1000;
	}
}


static struct timeval _tmv_diff(const struct timeval * left, const struct timeval * right)
{
	int64_t seconds = (int64_t)left->tv_sec - (int64_t)right->tv_sec;
	int32_t useconds = (int32_t)left->tv_usec - (int32_t)right->tv_usec;

	struct timeval retval = {
			.tv_sec = seconds,
			.tv_usec = useconds
	};

	_tmv_normalize(&retval);
	return retval;
}


static struct timeval _tmv_add(const struct timeval * left, const struct timeval * right)
{
	int64_t seconds = (int64_t)left->tv_sec + (int64_t)right->tv_sec;
	int32_t useconds = (int32_t)left->tv_usec + (int32_t)right->tv_usec;

	struct timeval retval = {
			.tv_sec = seconds,
			.tv_usec = useconds
	};

	_tmv_normalize(&retval);
	return retval;
}


void time_svc_init(void)
{
	// Просто включаем прерывания таймера и включаем его
	__HAL_TIM_ENABLE_IT(TIMESVC_TIM_HANDLE, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE(TIMESVC_TIM_HANDLE);

	// Таймер настроен так, что тикает 10 раз в секунду
	// и переполняется раз в секунду.
	// (частота на входе таймера 10 кГц, период 10*000 циклов)
}


uint8_t time_svc_get_time_base()
{
	return _my_timebase;
}


void time_svc_get_stats(time_svc_stats_t * stats)
{
	stats->syncs_attempted = _time_sync_attempts;
	stats->syncs_performed = _time_syncs_count;
	stats->last_sync_time_steady = _last_sync_ts;
	stats->last_sync_delta_s = _last_sync_delta_s;
	stats->last_sync_delta_us = _last_sync_delta_us;
}


uint16_t time_svc_get_time_syncs_count(void)
{
	return _time_syncs_count;
}


void time_svc_gettimeofday(struct timeval * tmv)
{
	time_t seconds, seconds2;
	suseconds_t subseconds;

	seconds = _seconds;
	subseconds = __HAL_TIM_GET_COUNTER(TIMESVC_TIM_HANDLE);
	seconds2 = _seconds;

	if (seconds != seconds2 && (subseconds & 0xFF00) == 0)
	{
		// Это значит что после того как мы прочитали переполнения
		// таймер переполнился и мы взяли значение счетчика для следующей итерации таймера
		// поэтому берем правильное значение переполнений
		seconds = seconds2;
	}

	tmv->tv_sec = seconds;
	tmv->tv_usec = subseconds * 100; // Таймер тикает с периодом в 100мкс
}


void time_svc_settimeofday(const struct timeval * tmv)
{
	// Останавливаем таймер
	__HAL_TIM_DISABLE(TIMESVC_TIM_HANDLE);

	// Ставим количество секунд
	_seconds = tmv->tv_sec;

	// Ставим счетчик таймеру
	const uint16_t cnt = tmv->tv_usec / 100;
	__HAL_TIM_SET_COUNTER(TIMESVC_TIM_HANDLE, cnt);

	// Запускаем таймер
	__HAL_TIM_ENABLE(TIMESVC_TIM_HANDLE);
}


void time_svc_on_tim_interrupt(void)
{
	_seconds++;
}


void time_svc_on_sync_interrupt(void)
{
	time_svc_gettimeofday(_last_sync_tmv_outer);
	_sync_pending = true;
}


void time_svc_on_mav_message(const mavlink_message_t * msg)
{
	// Нас интересуют только такие сообщения
	if (MAVLINK_MSG_ID_TIMESTAMP != msg->msgid)
		return;

	// Мы не ждали такого сообщения
	if (!_check_swap_sync())
		return;

	// Попытка началась
	_time_sync_attempts++;

	// Смотрим откуда хост взял это время вообще
	uint8_t time_base = mavlink_msg_timestamp_get_time_base(msg);
	if (time_base >= TIME_BASE_TYPE_ENUM_END)
		return;  // Это какая-то лажа

	// Если у нас качество метки лучше чем у хоста, то мы забиваем
	if (_my_timebase > time_base)
		return;

	const struct timeval * local_sync_stamp = _last_sync_tmv_inner;

	// Смотрим сколько времени было у хоста, когда он бросал нам sync метку
	const struct timeval host_sync_stamp = {
		.tv_sec = mavlink_msg_timestamp_get_time_s(msg),
		.tv_usec = mavlink_msg_timestamp_get_time_us(msg)
	};

	// Считаем разницу между временем у нас и у хоста на момент метки
	struct timeval diff;
	diff = _tmv_diff(&host_sync_stamp, local_sync_stamp);

	/*
	printf("host_time: %"PRId32", %"PRId32"\n", (int32_t)host_sync_stamp.tv_sec, host_sync_stamp.tv_usec);
	printf("local_time: %"PRId32", %"PRId32"\n", (int32_t)local_sync_stamp->tv_sec, local_sync_stamp->tv_usec);
	printf("time_sync_diff: %"PRId32", %"PRId32"\n", (int32_t)diff.tv_sec, diff.tv_usec);
	printf("=-=-=-=-\n");
	*/

	// Если разница меньше половины миллисекунды, то забиваем конечно
	if (diff.tv_sec == 0 && labs(diff.tv_usec) < 500)
		return;

	// Берем наше текущее время
	struct timeval current_time;
	time_svc_gettimeofday(&current_time);

	// добавляем разницу
	current_time = _tmv_add(&current_time, &diff);

	// Делаем это время текущим
	// FIXME: нужно бы добавить сколько-то микросекунд на все эти операции
	time_svc_settimeofday(&current_time);

	// Запоминаем наш таймбейс
	_my_timebase = time_base;

	// шалось удалась!
	_time_syncs_count++;
	_last_sync_delta_s = diff.tv_sec;
	_last_sync_delta_us = diff.tv_usec;
	_last_sync_ts = HAL_GetTick();
}

