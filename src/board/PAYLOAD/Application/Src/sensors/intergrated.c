
#include "sensors/integrated.h"

#include "time_svc.h"
#include "sensors/analog.h"


int integrated_read(mavlink_own_temp_t * msg)
{
	int error = 0;

	struct timeval tv;
	time_svc_gettimeofday(&tv);

	const int oversampling = 50;
	uint16_t raw;
	error = analog_get_raw(ANALOG_TARGET_INTEGRATED_TEMP, oversampling, &raw);
	if (0 != error)
		return error;

	// Пересчитываем по зашитым калибровочным коэффициентам
	const uint16_t val_c1 = *TEMPSENSOR_CAL1_ADDR;
	const uint16_t val_c2 = *TEMPSENSOR_CAL2_ADDR;
	const uint16_t t_c1 = TEMPSENSOR_CAL1_TEMP;
	const uint16_t t_c2 = TEMPSENSOR_CAL2_TEMP;

	const float slope = (float)(t_c2 - t_c1) / (val_c2 - val_c1);
	float temp = slope * (raw - val_c1) + t_c1;

	// Теперь посчитаем VBAT
	error = analog_get_raw(ANALOG_TARGET_VBAT, oversampling, &raw);
	if (0 != error)
		return error;

	float vbat = (raw * 2) * 3.3f / 0x0FFF; // * 2 потому что  VAT подключен к АЦП с делителем

	// Теперь vdda
	error = analog_get_vdda_mv(oversampling, &raw);
	if (0 != error)
		return error;

	float vdda = (float)raw / 1000;

	msg->time_s = tv.tv_sec;
	msg->time_us = tv.tv_usec;
	msg->time_steady = HAL_GetTick();

	msg->deg = temp;
	msg->vbat = vbat;
	msg->vdda = vdda;

	return 0;
}
