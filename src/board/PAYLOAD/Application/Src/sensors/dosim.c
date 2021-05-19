
#include "stm32f4xx_hal.h"

#include "sensors/dosim.h"

#include "util.h"
#include "time_svc.h"


extern  TIM_HandleTypeDef htim3;


typedef struct {
	uint32_t start_time_ms;
	TIM_HandleTypeDef * dosim_time;
} dosim_t;


static dosim_t data;


void dosim_init()
{
	data.start_time_ms = HAL_GetTick();
	data.dosim_time = &htim3;
	data.dosim_time->Instance->CNT = 0;
	HAL_TIM_Base_Start(data.dosim_time);
}


void dosim_read(mavlink_pld_dosim_data_t * msg)
{
	uint32_t stop_time_ms = HAL_GetTick();

	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;
	msg->count_tick = data.dosim_time->Instance->CNT;
	msg->delta_time = stop_time_ms - data.start_time_ms;
	data.dosim_time->Instance->CNT = 0;
	data.start_time_ms = stop_time_ms;

}
