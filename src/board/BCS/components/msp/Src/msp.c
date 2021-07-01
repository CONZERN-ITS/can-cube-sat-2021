/*
 * msp.c
 *
 *  Created on: Jul 1, 2021
 *      Author: sereshotes
 */


// Mid supply protection

#include "msp.h"
#include "msp_cfg.h"
#include "shift_reg.h"

typedef struct {
	int is_it_should_be_on;
	int is_it_really_on;
} msp_module_private_t;

msp_module_private_t mmp[MSP_MODULE_COUNT];
extern shift_reg_handler_t hsr;

int sorted[MSP_MODULE_COUNT];

void msp_init() {
	for (int i = 0; i < MSP_MODULE_COUNT; i++) {
		sorted[i] = i;
	}
	int test = 1;
	while (test) {
		test = 0;
		for (int i = 0; i < MSP_MODULE_COUNT - 1; i++) {
			if (msp_module_cfg_arr[sorted[i]].priority < msp_module_cfg_arr[sorted[i + 1]].priority) {
				int t = sorted[i];
				sorted[i] = sorted[i + 1];
				sorted[i + 1] = t;

				test = 1;
			}
		}
	}
}

void msp_rethink(uint32_t ticks) {
	int sum = 0;
	shift_reg_take(&hsr, ticks);
	for (int i = 0; i < MSP_MODULE_COUNT; i++) {
		int index = sorted[i];
		if (msp_module_cfg_arr[index].priority == 0) {
			break;
		}

		if (mmp[index].is_it_should_be_on && sum + msp_module_cfg_arr[index].power < MSP_MAX_POWER) {
			sum += msp_module_cfg_arr[index].power;
			mmp[index].is_it_really_on = 1;
		} else {
			mmp[index].is_it_really_on = 0;
		}
		shift_reg_set_level_pin(&hsr, msp_module_cfg_arr[index].pin, mmp[index].is_it_really_on);
	}
	shift_reg_return(&hsr);
	shift_reg_load(&hsr);
}


void msp_turn_on(msp_module_t module, int is_on) {
	mmp[module].is_it_should_be_on = is_on;
}

