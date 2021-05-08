/*
 * dna_temp.h
 *
 *  Created on: May 8, 2021
 *      Author: User
 */

#ifndef INC_SENSORS_DNA_TEMP_H_
#define INC_SENSORS_DNA_TEMP_H_

int read_dna_temp_value(uint16_t * raw_);

void calculate_temp(uint16_t raw, float * temp);

#endif /* INC_SENSORS_DNA_TEMP_H_ */
