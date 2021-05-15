/*
 * its_ccompressor.c
 *
 *  Created on: May 15, 2021
 *      Author: sereshotes
 */

#include "sensors/its_ccompressor.h"

#include "compressor-control.h"
#include "main.h"


typedef struct its_ccontrol_hal_t {
    ccontrol_hal_t base;

} its_ccontrol_hal_t;

static ccontrol_time_t ccontrol_get_time(ccontrol_hal_t * hal) {
    return HAL_GetTick();
}

static int ccontrol_pump_control(ccontrol_hal_t * hal, bool enable) {
    HAL_GPIO_WritePin(COMPR_ON_GPIO_Port, COMPR_ON_Pin, enable);
    return 0;
}
static int ccontrol_valve_control(ccontrol_hal_t * hal, bool open) {
    HAL_GPIO_WritePin(VALVE_ON_GPIO_Port, VALVE_ON_Pin, open);
    return 0;
}
static its_ccontrol_hal_t control = {
        .base = {.get_time = ccontrol_get_time,
                .pump_control = ccontrol_pump_control,
                .valve_control = ccontrol_valve_control
        }
};


void its_ccontrol_init() {
    ccontrol_init(&control.base);
}
