/*
 * dna_temp.h
 *
 *  Created on: May 8, 2021
 *      Author: User
 */

#ifndef INC_SENSORS_DNA_TEMP_H_
#define INC_SENSORS_DNA_TEMP_H_

#include "mavlink_main.h"

#include "main.h"

//! Данная функция считает температуру и управляет нагревателем
int dna_get_status(mavlink_pld_dna_data_t * msg);

#endif /* INC_SENSORS_DNA_TEMP_H_ */
