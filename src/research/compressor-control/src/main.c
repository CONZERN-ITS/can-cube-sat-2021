#include <stdbool.h>
#include <stdio.h>

#include "compressor-control.h"
#include "icao_table_calc.h"
#include "model.h"


typedef struct hal_t
{
	ccontrol_hal_t base;
	ccontrol_time_t now;
	bool valve_open;
	bool pump_on;
} hal_t;



static ccontrol_time_t get_time(ccontrol_hal_t * hal_)
{
	hal_t * const hal = (hal_t*)hal_;
	return hal->now;
}


static int pump_control(ccontrol_hal_t * hal_, bool enable)
{
	hal_t * const hal = (hal_t*)hal_;
	hal->pump_on = enable;

	return 0;
}


static int valve_control(ccontrol_hal_t * hal_, bool open)
{
	hal_t * const hal = (hal_t*)hal_;
	hal->valve_open = open;

	return 0;
}


int main()
{
	hal_t hal = {
			.base = {
					.get_time = get_time,
					.pump_control = pump_control,
					.valve_control = valve_control
			},
			.now = 0,
			.pump_on = false,
			.valve_open = false,
	};
	model_t model;

	ccontrol_init((ccontrol_hal_t*)&hal);
	model_ctor(&model);
	model_reset(&model);

	// Каждые сколько тактов будем писать строчку в отчеn
	const int report_period = 5;
	// Каждые сколько тактов будем оповещать упрваление компрессором
	const int notify_period = 2;
	// какое время считать за ноль
	const float seconds_offset = 212;

	// Заголвок .csv
	printf("minutes,seconds,altitude,inner_pressure,outer_pressure,state,pump_on,valve_open\n");
	while(model_step(&model) == 0)
	{
		float inner_pressure = model.inner_pressure;
		float outer_pressure = model.outer_pressure;
		float alt = model.altitude;
		int state = ccontrol_get_state();

		if (model.teak % notify_period == 0)
		{
			ccontrol_update_altitude(alt);
			ccontrol_update_inner_pressure(inner_pressure);
		}

		hal.now = model.time_ms;
		ccontrol_poll();

		model_notify_pump(&model, hal.pump_on);
		model_notify_valve(&model, hal.valve_open);

		float minutes = (model.time_ms / 1000.f + seconds_offset) / 60.f;
		float seconds = (model.time_ms / 1000.f + seconds_offset);
		if (model.teak % report_period == 0 || hal.pump_on)
		{
			printf(
					"%f,%f,%f,%f,%f,%d,%d,%d\n",
					minutes,
					seconds,
					alt,
					inner_pressure,
					outer_pressure,
					state,
					hal.pump_on,
					hal.valve_open
			);
		}
	};

	return 0;
}
