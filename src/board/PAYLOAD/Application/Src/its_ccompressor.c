/*
 * its_ccompressor.c
 *
 *  Created on: May 15, 2021
 *      Author: sereshotes
 */

#include "its_ccompressor.h"

#include <stdbool.h>

#include "compressor-control.h"
#include "time_svc.h"
#include "main.h"



typedef struct its_ccontrol_hal_t
{
	ccontrol_hal_t base;
	bool valve_open;
	bool pump_running;
} its_ccontrol_hal_t;


static ccontrol_time_t ccontrol_get_time(ccontrol_hal_t *hal)
{
	return HAL_GetTick();
}


static int ccontrol_pump_control(ccontrol_hal_t *hal_, bool enable)
{
	its_ccontrol_hal_t * const hal = (its_ccontrol_hal_t *) hal_;

	HAL_GPIO_WritePin(COMPR_ON_GPIO_Port, COMPR_ON_Pin, enable);
	hal->pump_running = enable;

	return 0;
}


static int ccontrol_valve_control(ccontrol_hal_t *hal_, bool open)
{
	its_ccontrol_hal_t * const hal = (its_ccontrol_hal_t *) hal_;

	HAL_GPIO_WritePin(VALVE_ON_GPIO_Port, VALVE_ON_Pin, open);
	hal->valve_open = open;

	return 0;
}


static its_ccontrol_hal_t control = {
	.base = {
		.get_time = ccontrol_get_time,
		.pump_control = ccontrol_pump_control,
		.valve_control = ccontrol_valve_control
	}
};


void its_ccontrol_init()
{
	ccontrol_init(&control.base);
	control.pump_running = false;
	control.valve_open = false;

	control.base.pump_control(&control.base, true);
	control.base.valve_control(&control.base, true);
	HAL_Delay(1000);
	control.base.pump_control(&control.base, false);
	control.base.valve_control(&control.base, false);
}


void its_ccontrol_update_altitude(float altitude)
{
	ccontrol_update_altitude(altitude);
}


void its_ccontrol_update_inner_pressure(float pressure)
{
	ccontrol_update_inner_pressure(pressure);
}


void its_ccontrol_work(void)
{
	ccontrol_poll();
}


void its_ccontrol_get_state(mavlink_pld_compressor_data_t * msg)
{
	struct timeval tmv;
	time_svc_gettimeofday(&tmv);

	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;
	msg->time_steady = HAL_GetTick();

	msg->pump_on = control.pump_running;
	msg->valve_open = control.valve_open;
}
