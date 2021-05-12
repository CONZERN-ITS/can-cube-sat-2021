#ifndef COMPRESSOR_CONTROL_H_
#define COMPRESSOR_CONTROL_H_

#include <stdint.h>
#include <stdbool.h>

typedef float ccontrol_alt_t;
typedef float ccontrol_pressure_t;
typedef uint32_t ccontrol_time_t;

struct ccontrol_hal_t;
typedef struct ccontrol_hal_t ccontrol_hal_t;

typedef ccontrol_time_t (*ccontrol_get_time_t)(ccontrol_hal_t * hal);
typedef int (*ccontrol_pump_control_t)(ccontrol_hal_t * hal, bool enable);
typedef int (*ccontrol_valve_control_t)(ccontrol_hal_t * hal, bool open);

struct ccontrol_hal_t
{
	ccontrol_get_time_t get_time;
	ccontrol_pump_control_t pump_control;
	ccontrol_valve_control_t valve_control;
};


typedef enum ccontrol_state_t
{
	//! Ждем очередного замера
	CCONTROL_STATE_IDLE = 1,
	//! Качаем компрессором!
	CCONTROL_STATE_PUMPING = 2,
	//! Проводим измерения
	CCONTROL_STATE_MEASURING = 3,
	//! Стравливаем давление и проветриваем камеру
	CCONTROL_STATE_DRAINING = 4,
	//! Отдыхаем после стравливания воздуха
	CCONTROL_STATE_COOLDOWN = 5,
} ccontrol_state_t;


int ccontrol_init(ccontrol_hal_t * hal);

ccontrol_state_t ccontrol_get_state(void);

int ccontrol_update_altitude(float altitude);

int ccontrol_update_inner_pressure(float pressure);

int ccontrol_poll(void);


#endif // COMPRESSOR_CONTROL_H_
