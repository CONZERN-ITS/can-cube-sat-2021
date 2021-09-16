/*
 * its_ccompressor.h
 *
 *  Created on: May 15, 2021
 *      Author: sereshotes
 */

#ifndef INC_SENSORS_ITS_CCOMPRESSOR_H_
#define INC_SENSORS_ITS_CCOMPRESSOR_H_


#include "mavlink_main.h"


void its_ccontrol_init() ;

void its_ccontrol_update_altitude(float altitude, bool data_from_ms5611);

void its_ccontrol_update_inner_pressure(float pressure);

void its_ccontrol_work(void);

void its_ccontrol_get_state(mavlink_pld_compressor_data_t * msg);

void its_ccontrol_pump_override(bool enable);

void its_ccontrol_valve_override(bool enable);

#endif /* INC_SENSORS_ITS_CCOMPRESSOR_H_ */
