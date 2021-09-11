/*
 * model.h
 *
 *  Created on: 10 июл. 2021 г.
 *      Author: snork
 */

#ifndef MODEL_H_
#define MODEL_H_


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct model_s
{
	uint32_t _teak;
	bool _pump_active;
	bool _valve_open;
	bool _going_up;

	void * _altitude_noise_generator;
	void * _inner_pressure_noise_generator;
	void * _outer_pressure_noise_generator;

	uint32_t time;

	float altitude;
	float altitude_error;

	float inner_pressure;
	float inner_pressure_error;

	float outer_pressure;
	float outer_pressure_error;

} model_t;


int model_ctor(model_t * model);

void model_dtor(model_t * model);

int model_notify_pump(model_t * model, bool pump_working);
int model_notify_valve(model_t * model, bool valve_open);

int model_step(model_t * model);

#endif /* MODEL_H_ */
