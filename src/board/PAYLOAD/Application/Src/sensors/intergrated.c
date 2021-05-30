
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

	const int oversampling = 10;
	uint32_t raw_sum = 0;
	uint16_t raw;
	for (int i = 0; i < oversampling; i++)
	{
		error = analog_get_raw(ANALOG_TARGET_INTEGRATED_TEMP, &raw);
		if (0 != error)
			return error;

		raw_sum += raw;
	}

	raw = raw_sum / oversampling;

	float mv = raw * 3300.0f / 0x0FFF;
	float temp = (mv - INTERNAL_TEMP_V25) / INTERNAL_TEMP_AVG_SLOPE + 25;

	msg->time_s = tv.tv_sec;
	msg->time_us = tv.tv_usec;
	msg->deg = temp;

	return 0;
}
