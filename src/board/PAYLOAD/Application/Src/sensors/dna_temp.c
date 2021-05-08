
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "sensors/dna_temp.h"
#include "sensors/analog.h"


// Нагрузочный резистор сенсора (в Омах)
#define DNA_TEMP_R_LOAD (22000.0f)
// Температура в градусах Цельсия
#define STANDART_TEMP (25.0f)
// Сопротивление термистора при 25 градусах Цельсия
#define DNA_TEMP_R_FOR_STANDART_TEMP (100000.0f)
// Коэффициент бета
#define SENSOR_COEFF_B (3950)


int read_dna_temp_value(uint16_t * raw_)
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

void calculate_temp(uint16_t raw, float * temp)
{
	float v_sens = (3.3f * raw) / 0x1000;
	float i = (3.3f - v_sens) / DNA_TEMP_R_LOAD;
	float rx = v_sens / i;

	(*temp) = 1 / (1 / (273.0f + STANDART_TEMP) - logf(DNA_TEMP_R_FOR_STANDART_TEMP / rx) / SENSOR_COEFF_B) - 273.0f;
}
