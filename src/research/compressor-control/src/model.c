#include "model.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include "icao_table_calc.h"
#include "random.h"



int model_ctor(model_t * model)
{
	memset(model, 0x00, sizeof(*model));

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


int model_reset(model_t * model)
{
	model->altitude = -100;
	model->_going_up = true;

	model->inner_pressure = 100*1000;
	model->outer_pressure = 100*1000;
	model->teak = 0;

	model->_valve_open = false;
	model->_pump_active = false;

	return 0;
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
	model->teak += 1;
	model->time_ms = model->teak * MODEL_TEAK_MS;
	if (model->time_ms > MODEL_TIME_LIMIT)
		return 1;

	if (model->_going_up)
	{
		model->altitude += MODEL_SPEED_UP * (MODEL_TEAK_MS/1000.f);

		if (model->altitude >= MODEL_MAX_ALT)
			model->_going_up = false;
	}
	else
	{
		model->altitude -= MODEL_SPEED_DOWN * (MODEL_TEAK_MS/1000.f);

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

	float delta_p = MODEL_LEAK_COEFF_K * fabs(model->inner_pressure - model->outer_pressure) + MODEL_LEAK_COEFF_B;
	model->inner_pressure -= delta_p;
	if (model->inner_pressure < model->outer_pressure)
		model->inner_pressure = model->outer_pressure;

	if (model->inner_pressure > model->outer_pressure * 2)
		model->inner_pressure = model->outer_pressure * 2;

	model->altitude_error = random_generator_get(model->_altitude_noise_generator);
	model->inner_pressure_error = random_generator_get(model->_inner_pressure_noise_generator);
	model->outer_pressure_error = random_generator_get(model->_outer_pressure_noise_generator);

	return 0;
}
