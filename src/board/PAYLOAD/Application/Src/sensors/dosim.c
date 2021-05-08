
#include "stm32f4xx_hal.h"

#include "sensors/dosim.h"


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
}


void dosim_read(dosim_data_t * output_data)
{
	uint32_t stop_time_ms = HAL_GetTick();
	output_data->count_tick = data.dosim_time->Instance->CNT;

	output_data->delta_time = stop_time_ms - data.start_time_ms;

	data.dosim_time->Instance->CNT = 0;
	data.start_time_ms = stop_time_ms;

}



