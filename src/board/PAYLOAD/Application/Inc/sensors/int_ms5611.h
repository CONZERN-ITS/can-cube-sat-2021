/*
 * int_ms5611.h
 *
 *  Created on: 17 апр. 2021 г.
 *      Author: developer
 */

#ifndef INC_SENSORS_INT_MS5611_H_
#define INC_SENSORS_INT_MS5611_H_

#include "mavlink_main.h"

extern struct prom_data int_ms5611_prom;

int int_ms5611_reset();

int int_ms5611_read_prom();

int int_ms5611_read_and_calculate(mavlink_pld_int_ms5611_data_t * data);

#endif /* INC_SENSORS_INT_MS5611_H_ */
