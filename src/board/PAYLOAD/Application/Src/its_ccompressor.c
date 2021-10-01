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

	uint8_t pump_on_cnt;
	uint32_t pump_on_ts;
	uint8_t pump_off_cnt;
	uint32_t pump_off_ts;
	uint8_t valve_open_cnt;
	uint32_t valve_open_ts;
	uint8_t valve_close_cnt;
	uint32_t valve_close_ts;
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
	if (enable)
	{
		hal->pump_on_cnt++;
		hal->pump_on_ts = HAL_GetTick();
	}
	else
	{
		hal->pump_off_cnt++;
		hal->pump_off_ts = HAL_GetTick();
	}

	return 0;
}


static int ccontrol_valve_control(ccontrol_hal_t *hal_, bool open)
{
	its_ccontrol_hal_t * const hal = (its_ccontrol_hal_t *) hal_;

	HAL_GPIO_WritePin(VALVE_ON_GPIO_Port, VALVE_ON_Pin, open);
	hal->valve_open = open;

	if (open)
	{
		hal->valve_open_cnt++;
		hal->valve_open_ts = HAL_GetTick();
	}
	else
	{
		hal->valve_close_cnt++;
		hal->valve_close_ts = HAL_GetTick();
	}

	return 0;
}


static its_ccontrol_hal_t control = {
	.base = {
		.get_time = ccontrol_get_time,
		.pump_control = ccontrol_pump_control,
		.valve_control = ccontrol_valve_control
	},
	0, 0,
	0, 0,
	0, 0,
	0, 0
};

static int last_time_get_ms5611_data = 0;

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


void its_ccontrol_update_altitude(float altitude, bool data_from_ms5611)
{
	if (data_from_ms5611)
	{
		last_time_get_ms5611_data = HAL_GetTick();
		ccontrol_update_altitude(altitude);
	}
	else
	{
		if ((last_time_get_ms5611_data - HAL_GetTick()) > 5000)
			ccontrol_update_altitude(altitude);
	}
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

	msg->state = 0;
	if (control.pump_running)
		msg->state |= (1 << 0);
	if (control.valve_open)
		msg->state |= (1 << 1);

	msg->pump_on = control.pump_on_cnt;
	msg->pump_on_time_steady = control.pump_on_ts;
	msg->pump_off = control.pump_off_cnt;
	msg->pump_off_time_steady = control.pump_off_ts;
	msg->valve_open = control.valve_open_cnt;
	msg->valve_open_time_steady = control.valve_open_ts;
	msg->valve_close = control.valve_close_cnt;
	msg->valve_close_time_steady = control.valve_close_ts;
}


void its_ccontrol_pump_override(bool enable)
{
	ccontrol_pump_control((ccontrol_hal_t *)&control, enable);
}


void its_ccontrol_valve_override(bool enable)
{
	ccontrol_valve_control((ccontrol_hal_t *)&control, enable);
}
