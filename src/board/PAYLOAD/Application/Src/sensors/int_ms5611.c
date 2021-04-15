#include "sensors/driver_ms5611.h"


extern I2C_HandleTypeDef hi2c1;
#define BUS_HANDLE &hi2c1


#define MS5611_BUS_RCC_FORCE_RESET __HAL_RCC_I2C1_FORCE_RESET
#define MS5611_BUS_RCC_RELEASE_RESET __HAL_RCC_I2C1_RELEASE_RESET

#define MS5611_I2C_ADDR MS5611_LOW_I2C_ADDRESS
#define MS5611_HAL_TIMEOUT (300)


#ifndef ITS_IMITATOR

static void _delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}



static int _int_ms5611_read_prom()
{
	return 0;
}

#endif

#ifdef ITS_IMITATOR
int bme_init()
{
	return 0;
}
#else
int int_ms5611_reset()
{
	int rc = 0;
	rc = ms5611_reset(BUS_HANDLE, MS5611_I2C_ADDR);

	if (0 != rc)
		return rc;

	HAL_Delay(10);
	return 0;
}
#endif
