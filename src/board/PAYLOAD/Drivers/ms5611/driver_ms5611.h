/*
 * driver_ms5611.h
 *
 *  Created on: Feb 5, 2021
 *      Author: developer
 */

#ifndef MS5611_DRIVER_MS5611_H_
#define MS5611_DRIVER_MS5611_H_


#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef ms5611_i2c;

struct prom_item {
	uint16_t value;
	uint8_t coef;
};

struct prom_data {
	uint16_t c1;
	uint16_t c2;
	uint16_t c3;
	uint16_t c4;
	uint16_t c5;
	uint16_t c6;
};

#endif /* MS5611_DRIVER_MS5611_H_ */
