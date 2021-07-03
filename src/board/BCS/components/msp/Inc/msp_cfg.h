/*
 * msp_cfg.h
 *
 *  Created on: Jul 1, 2021
 *      Author: sereshotes
 */

#ifndef COMPONENTS_MSP_INC_MSP_CFG_H_
#define COMPONENTS_MSP_INC_MSP_CFG_H_

#include "msp.h"
#include "pinout_cfg.h"

typedef struct {
	/*
	 * higher - better
	 * set 0 to turn it off completely
	 */
	int priority;
	/*
	 * pin of shift reg
	 */
	int pin;
	/*
	 * power of module
	 */
	int power;
} msp_module_cfg_t;

static msp_module_cfg_t msp_module_cfg_arr[MSP_MODULE_COUNT] = {
		[MSP_BCU_HEAT]  = {3, ITS_PIN_SR_BCU_HEAT, 300 * 8},

		[MSP_BSK1_VCC]  = {0, ITS_PIN_SR_BSK1_VCC, 0 * 8},
		[MSP_BSK1_ROZE] = {10, ITS_PIN_SR_BSK1_ROZE, 500 * 8},
		[MSP_BSK1_CMD]  = {10, ITS_PIN_SR_BSK1_CMD, 0 * 8},
		[MSP_BSK1_HEAT] = {1, ITS_PIN_SR_BSK1_HEAT, 300 * 8},

		[MSP_BSK2_VCC]  = {0, ITS_PIN_SR_BSK2_VCC, 0 * 8},
		[MSP_BSK2_ROZE] = {10, ITS_PIN_SR_BSK2_ROZE, 500 * 8},
		[MSP_BSK2_CMD]  = {10, ITS_PIN_SR_BSK2_CMD, 0 * 8},
		[MSP_BSK2_HEAT] = {1, ITS_PIN_SR_BSK2_HEAT, 300 * 8},

		[MSP_BSK3_VCC]  = {0, ITS_PIN_SR_BSK3_VCC, 0 * 8},
		[MSP_BSK3_ROZE] = {10, ITS_PIN_SR_BSK3_ROZE, 500 * 8},
		[MSP_BSK3_CMD]  = {10, ITS_PIN_SR_BSK3_CMD, 0 * 8},
		[MSP_BSK3_HEAT] = {1, ITS_PIN_SR_BSK3_HEAT, 300 * 8},

		[MSP_BSK4_VCC]  = {0, ITS_PIN_SR_BSK4_VCC, 0 * 8},
		[MSP_BSK4_ROZE] = {10, ITS_PIN_SR_BSK4_ROZE, 500 * 8},
		[MSP_BSK4_CMD]  = {10, ITS_PIN_SR_BSK4_CMD, 0 * 8},
		[MSP_BSK4_HEAT] = {1, ITS_PIN_SR_BSK4_HEAT, 300 * 8},

		[MSP_BSK5_HEAT]  = {2, ITS_PIN_SR_BSK5_HEAT, 300 * 8},
};


#endif /* COMPONENTS_MSP_INC_MSP_CFG_H_ */
