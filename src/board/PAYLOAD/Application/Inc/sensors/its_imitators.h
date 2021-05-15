/*
 * imitators.h
 *
 *  Created on: May 11, 2021
 *      Author: snork
 */

#ifndef SRC_SENSORS_IMITATORS_H_
#define SRC_SENSORS_IMITATORS_H_

#include <stdbool.h>

#include "mavlink_main.h"
#include "its_bme280.h"
#include "its_ms5611.h"

int its_imitators_process_input(void);

int its_imitators_bme280_read(its_bme280_id_t id, mavlink_pld_bme280_data_t * data);

int int_imitators_ms5611_read_and_calculate(its_ms5611_id_t id, mavlink_pld_ms5611_data_t * data);

#endif /* SRC_SENSORS_IMITATORS_H_ */
