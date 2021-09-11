#include "model.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include "icao_table_calc.h"
#include "random.h"


// Скорость подъема аппарата наверх
#define MODEL_SPEED_UP (4000.f/(60*1000)) // ~ 4км в 10 минут в м/c
// Скорость спуска аппарата вниз
#define MODEL_SPEED_DOWN (-13000.f/(60*1000)) // ~ -13 км за 10 минут в м/c
// Максимальная высота подъема после которой начинается спуск
#define MODEL_MAX_ALT (30*1000) // 30 км в м
// Минимальная высота
#define MODEL_MIN_ALT (-100)

// Мощность помпы. Сколько ПА в + к внешнему она может накачать
#define MODEL_PUMP_MAX_P (8*100*1000)
// Коэффициент накачки помпы. На каждом такте помпа накачивает k(p - p_max) + b,
// где p - текущее давление, p_max - максимальное давление которое может сделать помпа
// k - задается здесь
#define MODEL_PUMP_COEFF_K (0.5)
// Постоянная часть наканчки помпы (коээфициент b из k(p - p_max) + b)
#define MODEL_PUMP_COEFF_B (100)

// Коэффициент стравливания клапана. На каждом такте клапан сливает k(p - p_outer) + b,
// где p - текущее давление, p_outer - давление снаружи аппарата
// k - коэффициент этого параметра
#define MODEL_VALVE_COEFF_K (0.7)
// Постоянная часть травления клапана (коээфициент b из k(p - p_outer) + b)
#define MODEL_VALVE_COEFF_B (300)

// Длительность такта симуляции
#define MODEL_TEAK_MS (200) // 200 миллисекунд на такт симуляции
// Длительность симуляции
#define MODEL_TIME_LIMIT (2*60*60*1000) // два часа


int model_ctor(model_t * model)
{
	memset(model, 0x00, sizeof(*model));

	model->altitude = -100;
	model->_going_up = true;

	model->inner_pressure = 100*1000;
	model->outer_pressure = 100*1000;

	int seed = 42;
	model->_altitude_noise_generator = random_generator_ctor(seed, 0.0f, 5.0f);
	model->_inner_pressure_noise_generator = random_generator_ctor(seed, 0.0f, 50.0f);
	model->_outer_pressure_noise_generator = random_generator_ctor(seed, 0.0f, 50.0f);

	return 0;
}


void model_dtor(model_t * model)
{
	return;
}


int model_notify_pump(model_t * model, bool pump_working)
{
	model->_pump_active = pump_working;
	return 0;
}


int model_notify_valve(model_t * model, bool valve_open)
{
	model->_valve_open = valve_open;
	return 0;
}


int model_step(model_t * model)
{
	model->_teak += 1;
	model->time = model->_teak * MODEL_TEAK_MS;
	if (model->time > MODEL_TIME_LIMIT)
		return 1;

	if (model->_going_up)
		model->altitude += MODEL_SPEED_UP * (MODEL_TEAK_MS/1000.f);
	else
		model->altitude -= MODEL_SPEED_DOWN * (MODEL_TEAK_MS/1000.f);

	if (model->_going_up)
	{
		if (model->altitude >= MODEL_MAX_ALT)
			model->_going_up = false;
	}
	else
	{
		if (model->altitude < MODEL_MIN_ALT)
			model->altitude = MODEL_MIN_ALT; // Упали и лежим
	}

	model->outer_pressure = icao_table_pressure_for_alt(model->altitude);

	if (model->_pump_active)
	{
		float delta_p = MODEL_PUMP_COEFF_K*fabs(model->inner_pressure - MODEL_PUMP_MAX_P) + MODEL_PUMP_COEFF_B;
		model->inner_pressure += delta_p;
		if (model->inner_pressure > model->outer_pressure + MODEL_PUMP_MAX_P)
			model->inner_pressure = model->outer_pressure + MODEL_PUMP_MAX_P;
	}

	if (model->_valve_open)
	{
		float delta_p = MODEL_VALVE_COEFF_K * fabs(model->inner_pressure - model->outer_pressure) + MODEL_VALVE_COEFF_B;
		model->inner_pressure -= delta_p;
		if (model->inner_pressure < model->outer_pressure)
			model->inner_pressure = model->outer_pressure;
	}

	model->altitude_error = random_generator_get(model->_altitude_noise_generator);
	model->inner_pressure_error = random_generator_get(model->_inner_pressure_noise_generator);
	model->outer_pressure_error = random_generator_get(model->_outer_pressure_noise_generator);

	return 0;
}
