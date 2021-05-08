/*
 * dosim.h
 *
 *  Created on: May 8, 2021
 *      Author: User
 */

#ifndef INC_SENSORS_DOSIM_H_
#define INC_SENSORS_DOSIM_H_


typedef struct {
	// Количество насчитанных тиков
	uint32_t count_tick;
	// Время, за которое эти тики посчитаны
	uint32_t delta_time;
} dosim_data_t;


void dosim_read(dosim_data_t * output_data);

void dosim_init();

#endif /* INC_SENSORS_DOSIM_H_ */
