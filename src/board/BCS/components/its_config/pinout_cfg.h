/*
 * pinout_cfg.h
 *
 *  Created on: 13 апр. 2020 г.
 *      Author: sereshotes
 */

#ifndef MAIN_PINOUT_CFG_H_
#define MAIN_PINOUT_CFG_H_

#include "init_helper.h"


#define ITS_PIN_I2CTM_SCL 	16
#define ITS_PIN_I2CTM_SDA 	17
#define ITS_PIN_TIME		18


#define ITS_PIN_SPISR_SCK	22
#define ITS_PIN_SPISR_CS_SR	19
#define ITS_PIN_SPISR_CS_RA	21
#define ITS_PIN_SPISR_MOSI	23
#define ITS_PIN_SPISR_MISO	36

#define ITS_PIN_RADIO_DIO1	32
#define ITS_PIN_RADIO_RESET	33
#define ITS_PIN_RADIO_BUSY	35
#define ITS_PIN_RADIO_TX_EN	25
#define ITS_PIN_RADIO_RX_EN	26

#define ITS_PIN_LED			2
#define ITS_PIN_OWB			27

#endif /* MAIN_PINOUT_CFG_H_ */
