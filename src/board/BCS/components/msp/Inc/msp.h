/*
 * msp.h
 *
 *  Created on: Jul 1, 2021
 *      Author: sereshotes
 */

#ifndef COMPONENTS_MSP_INC_MSP_H_
#define COMPONENTS_MSP_INC_MSP_H_

#include "inttypes.h"
#include "freertos/FreeRTOS.h"

#define MSP_MAX_POWER (8 * (2300 - 300))


typedef enum {
	MSP_BCU_HEAT,

	MSP_BSK1_VCC,
	MSP_BSK1_ROZE,//500
	MSP_BSK1_CMD,
	MSP_BSK1_HEAT,

	MSP_BSK2_VCC,
	MSP_BSK2_ROZE,
	MSP_BSK2_CMD,
	MSP_BSK2_HEAT,

	MSP_BSK3_VCC,
	MSP_BSK3_ROZE,
	MSP_BSK3_CMD,
	MSP_BSK3_HEAT,

	MSP_BSK4_VCC,
	MSP_BSK4_ROZE,
	MSP_BSK4_CMD,
	MSP_BSK4_HEAT,

	MSP_BSK5_HEAT,

	MSP_MODULE_COUNT
} msp_module_t;



void msp_init();

BaseType_t msp_rethink(uint32_t ticks);

void msp_turn_on(msp_module_t module, int is_on);

#endif /* COMPONENTS_MSP_INC_MSP_H_ */
