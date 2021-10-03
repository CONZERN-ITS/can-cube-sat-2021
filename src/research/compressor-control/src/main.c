#include <stdbool.h>
#include <stdio.h>

#include "compressor-control.h"
#include "icao_table_calc.h"


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

	ccontrol_init((ccontrol_hal_t*)&hal);

	// Максимальная высота подъема (в м)
	const ccontrol_alt_t max_alt = 30*1000;
	// Скорость подъема (в м/мс)
	const ccontrol_alt_t alt_speed_up = 4000.f/(10 * 60*1000); // ~ 4км в 10 минут
	// Скорость спуска (в м/мс)
	const ccontrol_alt_t alt_speed_down = -13000.f/(10 * 60*1000); // ~ -13 км за 10 минут
	// Шаг симуляции по времени (в мс)
	const ccontrol_time_t time_step = 200;
	// Пауза в тактах между замерами высоты и оффсет этих замеров
	const uint32_t alt_measure_period = 4;
	const uint32_t alt_measure_offset = 0;
	// Пауза в тактах и между замерами внутренного давлени и оффсет этих замеров
	const uint32_t inner_pressure_measure_period = 4;
	const uint32_t inner_pressure_measure_offset = 1;
	// Длительность симуляции
	const uint64_t sym_time_limit = 80 * 60*1000;
	// Частота сообщений отчета (такты)
	const uint32_t report_period = 10;

	// Пока что мы летим вверх
	bool going_up = true;

	ccontrol_alt_t alt = -100;
	ccontrol_time_t time = 0;
	float outer_pressure_start = icao_table_pressure_for_alt(alt);
	ccontrol_pressure_t inner_pressure = outer_pressure_start;
	uint64_t tick;

	// Заголвок .csv
	printf("minutes,altitude,inner_pressure,outer_pressure,state,pump_on,valve_open\n");
	for (tick = 0; time < sym_time_limit; tick++)
	{
		time += time_step;
		if (going_up)
		{
			alt += alt_speed_up * time_step;
			if (alt >= max_alt)
				going_up = false; // начинаем падать
		}
		else
		{
			alt += alt_speed_down * time_step;
			if (alt <= -100)
				alt = -100;
		}

		float outer_pressure = icao_table_pressure_for_alt(alt);
		if (!hal.valve_open && hal.pump_on)
		{
			inner_pressure += 0.1*1000; // Внутреннее давление быстро растет!

			float pressure_cap = outer_pressure * 1.5;
			if (inner_pressure > pressure_cap)
				inner_pressure = pressure_cap;
		}
		else if (hal.valve_open)
		{
			// А так быстро падает
			inner_pressure -= 10*1000;
			if (inner_pressure < outer_pressure)
				inner_pressure = outer_pressure;
		}
		else
		{
			// Вот так утекает но не так быстро
			inner_pressure -= 0.1*1000;
			if (inner_pressure < outer_pressure)
				inner_pressure = outer_pressure;
		}

		hal.now = time;

		if (tick % alt_measure_period == alt_measure_offset)
			ccontrol_update_altitude(alt);

		if (tick % inner_pressure_measure_period == inner_pressure_measure_offset)
			ccontrol_update_inner_pressure(inner_pressure);

		ccontrol_poll();
		ccontrol_state_t state = ccontrol_get_state();

		if (tick % report_period == 0)
		{
			float minutes = (float)time/(60*1000);
			printf(
					"%f,%f,%f,%f,%d,%d,%d\n",
					minutes,
					alt,
					inner_pressure,
					outer_pressure,
					state,
					hal.pump_on,
					hal.valve_open
			);
		}
	}

	return 0;
}
