/*
 * my_i2c.h
 *
 *  Created on: 27 мар. 2020 г.
 *      Author: sereshotes
 */

#ifndef MAIN_INC_MY_I2C_H_
#define MAIN_INC_MY_I2C_H_

#include <stdint.h>

#include "driver/i2c.h"

BaseType_t i2c_master_prestart(i2c_port_t i2c_port, int timeout);

BaseType_t i2c_master_postend(i2c_port_t i2c_port);

int my_i2c_send(i2c_port_t i2c_port, uint8_t address, uint8_t *data, int size, int timeout);

int my_i2c_recieve(i2c_port_t i2c_port, uint8_t address, uint8_t *data, int size, int timeout);

void i2c_master_mutex_init(i2c_port_t i2c_port);

#endif /* MAIN_INC_MY_I2C_H_ */
