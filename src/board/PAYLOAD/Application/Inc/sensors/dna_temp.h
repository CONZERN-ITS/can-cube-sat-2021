/*
 * dna_temp.h
 *
 *  Created on: May 8, 2021
 *      Author: User
 */

#ifndef INC_SENSORS_DNA_TEMP_H_
#define INC_SENSORS_DNA_TEMP_H_

#include "mavlink_main.h"

#include "main.h"


//! Инициализация управления температурой образца ДНК
int dna_control_init(void);

//! Получение параметров образца ДНК и работы нагревателя
int dna_control_get_status(mavlink_pld_dna_data_t * msg);

//! Управление темературой образца ДНК
int dna_control_work(void);


#endif /* INC_SENSORS_DNA_TEMP_H_ */
