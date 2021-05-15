#include "compressor-control.h"

#include <string.h>

#include <assert.h>
#include <time.h>
#include <stdlib.h>


//! Высота с которой начинаем мерять (м)
#define CCONTROL_START_ALTITUDE (1*1000)
//! Шаг замеров по высоте (м)
#define CCONTROL_ALTITUDE_STEP  (100)
//! Внутреннее давление, на котором перестаем качать (Па)
#define CCONTROL_INNER_PRESSURE_CUTOFF (90*1000)
//! Таймаут в течение которого перестаем качать (мс)
#define CCONTROL_PUMP_TIMEOUT (4*1000)
//! Время в течение которого мы удерживаем давление в камере
#define CCONTROL_MEASURE_TIMEOUT (10*1000)
//! Время в течение которого мы проветриваем камеру
#define CCONTROL_DRAIN_TIMEOUT (5*1000)
//! Время в течение которого мы отдыхаем после очередного замера
#define CCONTROL_COOLDOWN_TIMEOUT (1*1000)


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
	ccontrol_alt_t next_pump_on_altitude;
	//! Точка времени включения компрессора
	ccontrol_time_t pump_on_timestamp;
	//! Точка времени начала замеров
	ccontrol_time_t measure_start_timestamp;
	//! Время начала стравливания воздуха
	ccontrol_time_t draing_start_timestamp;
	//! Время начала отдыха
	ccontrol_time_t cooldown_start_timestamp;

	ccontrol_hal_t * hal;
} ccontrol_t;


static ccontrol_t ccontrol;

void ccontrol_pull_generic_state(ccontrol_generic_state_t *cgs) {
    cgs->state = ccontrol.state;
    cgs->next_pump_on_altitude = ccontrol.next_pump_on_altitude;
    cgs->pump_on_timestamp = ccontrol.pump_on_timestamp;
    cgs->measure_start_timestamp = ccontrol.measure_start_timestamp;
    cgs->draing_start_timestamp = ccontrol.draing_start_timestamp;
    cgs->cooldown_start_timestamp = ccontrol.cooldown_start_timestamp;
}

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


static int _go_measure(ccontrol_t * const self)
{
	int rc;
	// Выключаем компрессор и неможечко ждем
	rc = self->hal->pump_control(self->hal, false);
	if (0 != rc)
		return rc;

	self->measure_start_timestamp = self->hal->get_time(self->hal);
	self->state = CCONTROL_STATE_MEASURING;

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


static int _go_cooldown(ccontrol_t * const self)
{
	int rc;

	// Выключаем насос
	rc = self->hal->pump_control(self->hal, false);
	if (0 != rc)
		return rc;

	// Открываем клапан (должен быть уже открыт, но мы параноики)
	rc = self->hal->valve_control(self->hal, true);
	if (0 != rc)
		return rc;

	self->cooldown_start_timestamp = self->hal->get_time(self->hal);
	self->state = CCONTROL_STATE_COOLDOWN;

	return 0;
}


static int _go_idle(ccontrol_t * const self)
{
	int rc;

	// Выключаем насос и открываем клапан
	rc = self->hal->pump_control(self->hal, false);
	if (0 != rc)
		return rc;

	rc = self->hal->valve_control(self->hal, true);
	if (0 != rc)
		return rc;

	self->state = CCONTROL_STATE_IDLE;

	return 0;
}


int ccontrol_init(ccontrol_hal_t * hal)
{
	ccontrol_t * const self = &ccontrol;

	memset(self, 0x00, sizeof(ccontrol_t));
	self->next_pump_on_altitude = CCONTROL_START_ALTITUDE;
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
		// Окей, мы качаем. Может уже набрали нужное давление?
		if (self->inner_pressure_updated)
		{
			self->inner_pressure_updated = false;
			if (self->inner_pressure >= CCONTROL_INNER_PRESSURE_CUTOFF)
			{
				// Накачали!
				_go_measure(self);
				break;
			}
		}

		// Нужного давления все еще нет, может пора заканчивать по таймауту?
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->pump_on_timestamp > CCONTROL_PUMP_TIMEOUT)
		{
			// да, пора к сожалению
			_go_measure(self);
			break;
		}

		// нет, пока продолжим накачивать
	} break;

	case CCONTROL_STATE_MEASURING: {
		// Так, мы измеряем. Не пора ли сворачиваться?
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->measure_start_timestamp > CCONTROL_MEASURE_TIMEOUT)
		{
			// Пора
			_go_drain(self);
			break;
		}

		// Нет, не пора, еще потупим
	} break;

	case CCONTROL_STATE_DRAINING: {
		// Ну... тут не знаю как быть.
		// Поидее можно дождаться выравнивания давления?
		// Пока что сделаем просто таймаут
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->draing_start_timestamp > CCONTROL_DRAIN_TIMEOUT)
		{
			// Пора завязывать
			_go_cooldown(self);
			break;
		}

		// Нет, еще пускай по-проветривается
	} break;

	case CCONTROL_STATE_COOLDOWN: {
		// Отдыхаем
		ccontrol_time_t now = self->hal->get_time(self->hal);
		if (now - self->cooldown_start_timestamp > CCONTROL_COOLDOWN_TIMEOUT)
		{
			// Хватит отдыхать
			_go_idle(self);
			break;
		}

		// Все еще отдыхаем
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
		if (self->altitude > self->next_pump_on_altitude)
		{
			// Ага, мы поднялись достаточно высоко, чтобы сделать следующий замер
			// Если мы сейчас свободны, то пожалуй ка его и начнем
			if (CCONTROL_STATE_IDLE == self->state)
				_go_pump(self);

			// В любом случае переключаем метку для следующего включения
			self->next_pump_on_altitude += CCONTROL_ALTITUDE_STEP;
		}
	}

	return 0;
}
