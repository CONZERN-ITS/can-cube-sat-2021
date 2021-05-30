
#include "sensors/integrated.h"

#include "time_svc.h"
#include "sensors/analog.h"


// Напряжение со встроенного термистра при 25 градусах (в милливольтах)
#define INTERNAL_TEMP_V25 (760.0f)
// Коэффициент k внутренного термистра (мВ/C)
#define INTERNAL_TEMP_AVG_SLOPE (2.5f)


int integrated_read(mavlink_own_temp_t * msg)
{
	int error = 0;

	struct timeval tv;
	time_svc_gettimeofday(&tv);

	const int oversampling = 50;
	uint32_t raw_sum = 0;
	uint16_t raw;
	uint32_t vdda_sum = 0;
	uint16_t vdda;
	for (int i = 0; i < oversampling; i++)
	{
		error = analog_get_raw_with_vdda(ANALOG_TARGET_INTEGRATED_TEMP, &raw, &vdda);
		if (0 != error)
			return error;

		// Компенсируем с учетом входного напряжения
		raw = raw * vdda / ANALOG_VDDA_NOMINAL;
		raw_sum += raw;

		vdda_sum += vdda;
	}

	raw = raw_sum / oversampling;
	vdda = vdda_sum / oversampling;

	// Пересчитываем по зашитым калибровочным коэффициентам
	const uint16_t val_c1 = *TEMPSENSOR_CAL1_ADDR;
	const uint16_t val_c2 = *TEMPSENSOR_CAL2_ADDR;
	const uint16_t t_c1 = TEMPSENSOR_CAL1_TEMP;
	const uint16_t t_c2 = TEMPSENSOR_CAL2_TEMP;

	const float slope = (float)(t_c2 - t_c1) / (val_c2 - val_c1);
	float temp = slope * (raw - val_c1) + t_c1;

	msg->time_s = tv.tv_sec;
	msg->time_us = tv.tv_usec;
	msg->deg = temp;

	return 0;
}
