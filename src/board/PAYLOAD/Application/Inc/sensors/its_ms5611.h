/*
 * int_ms5611.h
 *
 *  Created on: 17 апр. 2021 г.
 *      Author: developer
 */

#ifndef INC_SENSORS_ITS_MS5611_H_
#define INC_SENSORS_ITS_MS5611_H_


#include <stdbool.h>

#include <stm32f4xx_hal.h>

#include "main.h"

#include "mavlink_main.h"


typedef enum its_ms5611_id_t
{
	ITS_MS_EXTERNAL = 0,
	ITS_MS_INTERNAL = 1,
} its_ms5611_id_t;


struct its_ms5611_t;
typedef struct its_ms5611_t its_ms5611_t;


int int_ms5611_reinit(its_ms5611_id_t id);

int its_ms5611_power(its_ms5611_id_t id, bool enabled);

int int_ms5611_read_and_calculate(its_ms5611_id_t id, mavlink_pld_ms5611_data_t * data);


#endif /* INC_SENSORS_ITS_MS5611_H_ */
