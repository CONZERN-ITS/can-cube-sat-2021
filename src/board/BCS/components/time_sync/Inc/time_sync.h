/*
 * time_sync.h
 *
 *  Created on: Jul 14, 2020
 *      Author: sereshotes
 */

#ifndef COMPONENTS_TIME_SYNC_INC_TIME_SYNC_H_
#define COMPONENTS_TIME_SYNC_INC_TIME_SYNC_H_


#include "driver/gpio.h"
#include "hal/i2c_types.h"
#include "soc/i2c_caps.h"

#include "lwip/ip_addr.h"
#include <esp_types.h>
#include <stdatomic.h>

#define TIME_SYNC_SMOOTH_THREASHOLD 60 //секунд



typedef struct {
	gpio_num_t pin;
	int coef;
	int period;

} ts_cfg;

void time_sync_from_sins_install(ts_cfg *cfg);

void time_sync_from_bcs_install(const ip_addr_t *server_ip);

uint8_t time_sync_get_base();


#endif /* COMPONENTS_TIME_SYNC_INC_TIME_SYNC_H_ */
