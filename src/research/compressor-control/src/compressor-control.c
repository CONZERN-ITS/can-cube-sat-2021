#include "compressor-control.h"

#include <string.h>

#include <assert.h>
#include <time.h>
#include <stdlib.h>


//! Высота с которой начинаем мерять (м)
#define CCONTROL_START_ALTITUDE (1*1000)
//! Шаг замеров по высоте (м)
#define CCONTROL_ALTITUDE_STEP  (1000)
//! Внутреннее давление, на котором перестаем качать (Па)
#define CCONTROL_INNER_PRESSURE_CUTOFF_UPPER (90*1000)
//! Внутреннее давление, на котором начинаем подкачивать (Па)
#define CCONTROL_INNER_PRESSURE_CUTOFF_LOWER (85*1000)
//! Таймаут в течение которого перестаем качать (мс)
#define CCONTROL_PUMP_TIMEOUT (60*1000)
//! Время в течение которого мы проветриваем камеру (мс)
#define CCONTROL_DRAIN_TIMEOUT (10*1000)
//! Максимальная высота на которой мы еще пытаемся нагнетать воздух (м)
#define CCONTROL_MAX_PUMP_ALTITUDE (12*1000)


typedef struct ccontrol_t
{
	//! текущая высота
	ccontrol_alt_t altitude;
	bool altitude_updated;

	//! Текущее давление во внутренней камере
	ccontrol_pressure_t inner_pressure;
	bool inner_pressure_updated;

	//! Состояние модуля
	ccontrol_state_t state;
	//! Высота на которой нужно сделать следующее включение компрессора
	ccontrol_alt_t next_cycle_altitude;
	//! Точка времени включения компрессора
	ccontrol_time_t pump_on_timestamp;
	//! Время начала стравливания воздуха
	ccontrol_time_t draing_start_timestamp;

	ccontrol_hal_t * hal;
} ccontrol_t;


static ccontrol_t ccontrol;


static int _go_pump(ccontrol_t * const self)
{
	int rc;

	// Закрываем клапаны
	rc = self->hal->valve_control(self->hal, false);
	if (0 != rc)
		return rc; // Ой ой, ну мы попробуем в следующий раз

	// Включаем компрессор!
	rc = self->hal->pump_control(self->hal, true);
	if (0 != rc)
		return rc; // Он не включился... остаемся в этом состоянии. будем пытаться еще и еще

	// Запоминаем когда мы включили компрессор
	self->pump_on_timestamp = self->hal->get_time(self->hal);
	// Переходим в соответствующее состояние
	self->state = CCONTROL_STATE_PUMPING;

	return 0;
}


static int _go_drain(ccontrol_t * const self)
{
	int rc;

	// Открываем клапаны и включаем насос. Именно в таком порядке!
	rc = self->hal->valve_control(self->hal, true);
	if (0 != rc)
		return rc;

	rc = self->hal->pump_control(self->hal, true);
	if (0 != rc)
		return rc;

	self->draing_start_timestamp = self->hal->get_time(self->hal);
	self->state = CCONTROL_STATE_DRAINING;

	return 0;
}


static int _go_idle(ccontrol_t * const self)
{
	int rc;

	// Выключаем насос и закрываем клапан
	rc = self->hal->pump_control(self->hal, false);
	if (0 != rc)
		return rc;

	rc = self->hal->valve_control(self->hal, false);
	if (0 != rc)
		return rc;

	self->state = CCONTROL_STATE_IDLE;

	return 0;
}


int ccontrol_init(ccontrol_hal_t * hal)
{
	ccontrol_t * const self = &ccontrol;

	memset(self, 0x00, sizeof(ccontrol_t));
	self->next_cycle_altitude = CCONTROL_START_ALTITUDE;
	self->state = CCONTROL_STATE_IDLE;
	self->hal = hal;
	return 0;
}


ccontrol_state_t ccontrol_get_state(void)
{
	ccontrol_t * const self = &ccontrol;
	return self->state;
}


int ccontrol_update_altitude(float altitude)
{
	ccontrol_t * const self = &ccontrol;

	self->altitude = altitude;
	self->altitude_updated = true;

	return 0;
}


int ccontrol_update_inner_pressure(float pressure)
{
	ccontrol_t * const self = &ccontrol;

	self->inner_pressure = pressure;
	self->inner_pressure_updated = true;

	return 0;
}


int ccontrol_poll(void)
{
	ccontrol_t * const self = &ccontrol;

	switch (self->state)
	{
	case CCONTROL_STATE_IDLE: {
		// IDLE мы рассматриваем отдельно, после свича
	} break;

	case CCONTROL_STATE_PUMPING: {
		// Смотрим что у нас с таймаутом на работу
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->pump_on_timestamp > CCONTROL_PUMP_TIMEOUT)
		{
			// да, пора к сожалению
			_go_idle(self);
			break;
		}

		// Окей, мы качаем. Может уже набрали нужное давление?
		if (self->inner_pressure_updated)
		{
			self->inner_pressure_updated = false;
			if (self->inner_pressure >= CCONTROL_INNER_PRESSURE_CUTOFF_UPPER)
			{
				// Накачали!
				// Выклюим пока что помпу
				self->hal->pump_control(self->hal, false);
				// Останавливаться на этом не будем, так как давление в гермообъеме неплохо падает
				// И возможно нам еще придется его подкачивать
			}
			else if (self->inner_pressure <= CCONTROL_INNER_PRESSURE_CUTOFF_LOWER)
			{
				// Сдулось. Докачаем
				self->hal->pump_control(self->hal, true);
				// Так же, состояние покидаем только по таймауту
				// Потому что потом помпу нужно будет выклчить
			}
		}
	} break;

	case CCONTROL_STATE_DRAINING: {
		// Ждем пока камера проверится. Уже пора?
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->draing_start_timestamp > CCONTROL_DRAIN_TIMEOUT)
		{
			// Пора завязывать
			if (self->altitude <= CCONTROL_MAX_PUMP_ALTITUDE)
			{
				// Начинаем качать, если еще смысл качать по высоте
				_go_pump(self);
			}
			else
			{
				// возвращаемся в idle состояние, если качать уже бессмыслеено
				_go_idle(self);
			}
			break;
		}

		// Нет, еще пускай по-проветривается
	} break;

	default: {
		// Вот этого быть не должно
		abort();
	} break;

	}; // switch


	// IDLE состояние рассматриваем отдельного от большого свича
	if (self->altitude_updated)
	{
		self->altitude_updated = false;
		if (self->altitude > self->next_cycle_altitude)
		{
			// Ага, мы поднялись достаточно высоко, чтобы сделать следующий замер
			// Если мы сейчас свободны, то пожалуй ка его и начнем
			if (CCONTROL_STATE_IDLE == self->state)
				_go_drain(self);

			// В любом случае переключаем метку для следующего включения
			self->next_cycle_altitude += CCONTROL_ALTITUDE_STEP;
		}
	}

	return 0;
}
