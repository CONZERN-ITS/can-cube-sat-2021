#include "sensors/driver_ms5611.h"
#include "sensors/int_ms5611.h"

#include "time_svc.h"
#include "util.h"

extern I2C_HandleTypeDef hi2c1;
#define BUS_HANDLE &hi2c1


#define MS5611_BUS_RCC_FORCE_RESET __HAL_RCC_I2C1_FORCE_RESET
#define MS5611_BUS_RCC_RELEASE_RESET __HAL_RCC_I2C1_RELEASE_RESET

#define INT_MS5611_I2C_ADDR MS5611_LOW_I2C_ADDRESS
#define MS5611_HAL_TIMEOUT (300)

struct prom_data int_ms5611_prom = {0};


#ifndef ITS_IMITATOR

static void _delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}
#endif

#ifdef ITS_IMITATOR
int int_ms5611_reset()
{
	return 0;
}
#else
int int_ms5611_reset()
{
	int rc = 0;
	rc = ms5611_reset(BUS_HANDLE, INT_MS5611_I2C_ADDR);

	_delay_ms(10);
	return rc;
}
#endif


#ifdef ITS_IMITATOR
int int_ms5611_read_prom()
{
	return 0;
}
#else
int int_ms5611_read_prom()
{
	int rc = 0;
	rc = ms5611_read_all_prom_data(BUS_HANDLE, INT_MS5611_I2C_ADDR, START_PROM_READ_ADDRESS, &int_ms5611_prom);
	return rc;
}
#endif


#ifdef ITS_IMITATOR
int _int_ms5611_read_pressure(uint32_t * raw_pressure)
{
	*raw_pressure = 0;
	return 0;
}
#else
int _int_ms5611_read_pressure(uint32_t * raw_pressure)
{
	int rc = 0;
	rc = ms5611_initiate_conversion(BUS_HANDLE, INT_MS5611_I2C_ADDR, CMD_CONVERT_PRESSURE_OSR_4096);
	if (0 != rc)	return rc;
	_delay_ms(10);

	rc = ms5611_read_data(BUS_HANDLE, INT_MS5611_I2C_ADDR, raw_pressure);

	return rc;
}
#endif


#ifdef ITS_IMITATOR
int _int_ms5611_read_temperature(uint32_t * raw_temperature)
{
	*raw_temperature = 0;
	return 0;
}
#else
int _int_ms5611_read_temperature(uint32_t * raw_temperature)
{
	int rc = 0;
	rc = ms5611_initiate_conversion(BUS_HANDLE, INT_MS5611_I2C_ADDR, CMD_CONVERT_TEMP_OSR_4096);
	if (0 != rc)	return rc;
	_delay_ms(10);

	rc = ms5611_read_data(BUS_HANDLE, INT_MS5611_I2C_ADDR, raw_temperature);

	return rc;
}
#endif


#ifdef ITS_IMITATOR
int int_ms5611_read_and_calculate(mavlink_pld_int_ms5611_data_t * data)
{
	struct timeval tv;
	time_svc_gettimeofday(&tv);

	data->time_s = tv.tv_sec;
	data->time_us = tv.tv_usec;
	data->pressure = 100*1000;
	data->temperature = 366;

	return 0;
}
#else
int int_ms5611_read_and_calculate(mavlink_pld_int_ms5611_data_t * data)
{
	int rc = 0;
	uint32_t raw_temp = 0, raw_pressure = 0;
	int32_t temp = 0, pressure = 0;

	rc = _int_ms5611_read_pressure(&raw_pressure);
	if (0 != rc)	return rc;

	rc = _int_ms5611_read_temperature(&raw_temp);
	if (0 != rc)	return rc;

	ms5611_calculate_temp_and_pressure(&raw_temp, &raw_pressure, &int_ms5611_prom, &temp, &pressure);

	struct timeval tv;
	time_svc_gettimeofday(&tv);

	data->time_s = tv.tv_sec;
	data->time_us = tv.tv_usec;
	data->pressure = pressure;
	data->temperature = temp;

	return rc;
}
#endif




