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

typedef struct ccontrol_generic_state_t {
    //! Состояние модуля
    ccontrol_state_t state;
    //! Высота на которой нужно сделать следующее включение компрессора
    ccontrol_alt_t next_pump_on_altitude;
    //! Точка времени включения компрессора
    ccontrol_time_t pump_on_timestamp;
    //! Точка времени начала замеров
    ccontrol_time_t measure_start_timestamp;
    //! Время начала стравливания воздуха
    ccontrol_time_t draing_start_timestamp;
    //! Время начала отдыха
    ccontrol_time_t cooldown_start_timestamp;
} ccontrol_generic_state_t;


int ccontrol_init(ccontrol_hal_t * hal);

void ccontrol_pull_generic_state(ccontrol_generic_state_t *cgs);

ccontrol_state_t ccontrol_get_state(void);

int ccontrol_update_altitude(float altitude);

int ccontrol_update_inner_pressure(float pressure);

int ccontrol_poll(void);


#endif // COMPRESSOR_CONTROL_H_
