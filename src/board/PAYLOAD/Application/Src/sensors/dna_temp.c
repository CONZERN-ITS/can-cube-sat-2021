
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "sensors/dna_temp.h"
#include "sensors/analog.h"

#include "util.h"
#include "time_svc.h"


// Нагрузочный резистор сенсора (в Омах) с погрешностью
#define DNA_TEMP_R_LOAD (22160.0f)
// Температура в градусах Цельсия
#define STANDART_TEMP (25.0f)
// Сопротивление термистора при 25 градусах Цельсия
#define DNA_TEMP_R_FOR_STANDART_TEMP (100000.0f)
// Коэффициент бета
#define SENSOR_COEFF_B (3950)

//! Температура, ниже которой необходимо включать нагреватель
#define MIN_DNA_TEMP_WITHOUT_HEATER (30.0)
//! Температура, выше которой необходимо выключать нагреватель
#define MAX_DNA_TEMP_WITH_HEATER (40.0)


typedef struct {
	float dna_temp;
	uint8_t heater_is_on;
} dna_data_t;


static dna_data_t data;


static void dna_on_heater()
{
	HAL_GPIO_WritePin(HEATER_PWR_GPIO_Port, HEATER_PWR_Pin, SET);
	data.heater_is_on = true;
}


static void dna_off_heater()
{
	HAL_GPIO_WritePin(HEATER_PWR_GPIO_Port, HEATER_PWR_Pin, RESET);
	data.heater_is_on = false;
}


int dna_control_init()
{
	dna_off_heater();
	return 0;
}


static int dna_read_temp_value(uint16_t * raw_)
{
	// делаем замер через АЦП
	// Несколько замеров, чтобы фильтрануть шум
	int error = 0;
	uint16_t raw = 0;
	uint32_t raw_sum = 0;
	const int oversampling = 10;
	for (int i = 0; i < oversampling; i++)
	{
		error = analog_get_raw(ANALOG_TARGET_DNA_TEMP, &raw);
		if (0 != error)
			return error;

		raw_sum += raw;
	}

	(*raw_) = raw_sum / oversampling;
	return 0;
}


static void dna_calculate_temp(const uint16_t raw, float * celsium)
{
	float v_sens = (3.3f * raw) / 0x1000;
	float i = (3.3f - v_sens) / DNA_TEMP_R_LOAD;
	float rx = v_sens / i;

	*celsium = 1 / (1 / (273.0f + STANDART_TEMP) - logf(DNA_TEMP_R_FOR_STANDART_TEMP / rx) / SENSOR_COEFF_B) - 273.0f;
}


int dna_control_get_status(mavlink_pld_dna_data_t * msg)
{
	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;

	msg->dna_temp = data.dna_temp;
	msg->heater_is_on = data.heater_is_on;

	return 0;
}


int dna_control_work()
{
	uint16_t raw_temp = 0;
	int error = 0;
	// Получаем сырое значение температуры
	error = dna_read_temp_value(&raw_temp);
	if (0 != error)
		return error;

	dna_calculate_temp(raw_temp, &data.dna_temp);

	if (MIN_DNA_TEMP_WITHOUT_HEATER > data.dna_temp)
		dna_on_heater();
	else if (MAX_DNA_TEMP_WITH_HEATER < data.dna_temp)
		dna_off_heater();

	return 0;
}
