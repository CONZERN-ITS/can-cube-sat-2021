/*
 * control_vcc.c
 *
 *  Created on: Aug 8, 2020
 *      Author: sereshotes
 */


#include "control_vcc.h"

#include "esp_log.h"

#include "shift_reg.h"
#include "init_helper.h"



const static int _shift = 1;



static shift_reg_handler_t *_hsr;
#define ITS_PIN_SR_SINS_VCC 17
#define ITS_PIN_SR_PL_VCC 18


void control_vcc_init(shift_reg_handler_t *hsr, int shift, uint32_t pl_pin) {
	//_shift = shift;
	_hsr = hsr;
	for (int i = 0; i < 5; i++) {
		shift_reg_set_level_pin(_hsr, _shift + i * 4, 1);
	}
}
void control_vcc_bsk_enable(int bsk_number, int is_on) {
	shift_reg_set_level_pin(_hsr, _shift + bsk_number * 4, is_on > 0);
}
void control_vcc_sins_enable(int is_on) {
	shift_reg_set_level_pin(_hsr, ITS_PIN_SR_SINS_VCC, is_on > 0);
}

void control_vcc_pl_enable(int is_on) {
	//shift_reg_set_level_pin(_hsr, ITS_PIN_SR_PL_VCC + pl_number, is_on > 0);
}
