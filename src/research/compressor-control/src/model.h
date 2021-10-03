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


// Скорость подъема аппарата наверх
#define MODEL_SPEED_UP ((4*1000.f)/(10*60)) // ~ 4км в 10 минут в м/c
// Скорость спуска аппарата вниз
#define MODEL_SPEED_DOWN ((13*1000.f)/(10*60)) // ~ -13 км за 10 минут в м/c
// Максимальная высота подъема после которой начинается спуск
#define MODEL_MAX_ALT (30*1000) // 30 км в м
// Минимальная высота
#define MODEL_MIN_ALT (-100)

// Длительность такта симуляции
#define MODEL_TEAK_MS (200) // 200 миллисекунд на такт симуляции

// Мощность помпы. Сколько ПА в + к внешнему она может накачать
#define MODEL_PUMP_MAX_P (1*100*1000)
// Коэффициент накачки помпы. На каждом такте помпа накачивает k(p - p_max) + b,
// где p - текущее давление, p_max - максимальное давление которое может сделать помпа
// k - задается здесь
#define MODEL_PUMP_COEFF_K (0)
// Постоянная часть наканчки помпы (коээфициент b из k(p - p_max) + b)
#define MODEL_PUMP_COEFF_B ((47*1000)/(1000/MODEL_TEAK_MS)) // тест показали при нормальном давлении ~46 кПа/с

// Коэффициент стравливания открытого клапана. На каждом такте клапан сливает k(p - p_outer) + b,
// где p - текущее давление, p_outer - давление снаружи аппарата
// k - коэффициент этого параметра
#define MODEL_VALVE_COEFF_K (0.3)
// Постоянная часть травления открытого клапана (коээфициент b из k(p - p_outer) + b)
#define MODEL_VALVE_COEFF_B ((10*1000)/(1000/MODEL_TEAK_MS))

// Коэффициент постоянного травления. На каждом такте сливается k(p - p_outer) + b,
// где p - текущее давление, p_outer - давление снаружи аппарата
#define MODEL_LEAK_COEFF_K (0)
// Постоянная часть постоянного травления всего (коээфициент b из k(p - p_outer) + b)
#define MODEL_LEAK_COEFF_B ((1.46*1000)/(1000/MODEL_TEAK_MS)) // тесты показали 1.46 кПа/с

// Длительность симуляции
#define MODEL_TIME_LIMIT (1*60*60*1000) // два часа
//#define MODEL_TIME_LIMIT (2*60*60*1000) // два часа




typedef struct model_s
{
	uint32_t teak;

	void * _altitude_noise_generator;
	void * _inner_pressure_noise_generator;
	void * _outer_pressure_noise_generator;

	uint32_t time_ms;

	float altitude;
	float altitude_error;

	float inner_pressure;
	float inner_pressure_error;

	float outer_pressure;
	float outer_pressure_error;

	bool _pump_active;
	bool _valve_open;
	bool _going_up;
} model_t;


int model_ctor(model_t * model);

void model_dtor(model_t * model);

int model_reset(model_t * model);

int model_notify_pump(model_t * model, bool pump_working);
int model_notify_valve(model_t * model, bool valve_open);

int model_step(model_t * model);

#endif /* MODEL_H_ */
