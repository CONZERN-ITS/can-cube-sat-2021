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
#define ITS_PIN_I2CTM_TIME	18
#define ITS_PIN_PPS			39


#define ITS_PIN_SPISR_SCK	22
#define ITS_PIN_SPISR_CS_SR	19
#define ITS_PIN_SPISR_CS_RA	21
#define ITS_PIN_SPISR_MOSI	23
#define ITS_PIN_SPISR_MISO	36 //VP

#define ITS_PIN_RADIO_DIO1	32
#define ITS_PIN_RADIO_RESET	33
#define ITS_PIN_RADIO_BUSY	35
#define ITS_PIN_RADIO_TX_EN	25
#define ITS_PIN_RADIO_RX_EN	26

#define ITS_PIN_LED			5
#define ITS_PIN_OWB			12

//-------------------------------------------
#define ITS_PIN_SR_BSK1_VCC  9
#define ITS_PIN_SR_BSK2_VCC  13
#define ITS_PIN_SR_BSK3_VCC  25
#define ITS_PIN_SR_BSK4_VCC  29

#define ITS_PIN_SR_SD_VCC    17
#define ITS_PIN_SR_RADIO_VCC 18
#define ITS_PIN_SR_SINS_VCC  1
#define ITS_PIN_SR_PL1_VCC   2
#define ITS_PIN_SR_PL2_VCC   3
#define ITS_PIN_SR_PL3_VCC   4
#define ITS_PIN_SR_PL4_VCC   5
#define ITS_PIN_SR_PL5_VCC   6
#define ITS_PIN_SR_PL6_VCC   7

#define ITS_PIN_SR_BSK1_HEAT 8
#define ITS_PIN_SR_BSK2_HEAT 12
#define ITS_PIN_SR_BSK3_HEAT 24
#define ITS_PIN_SR_BSK4_HEAT 28
#define ITS_PIN_SR_BSK5_HEAT 0
#define ITS_PIN_SR_BCU_HEAT  16

#define ITS_PIN_SR_BSK1_ROZE 10
#define ITS_PIN_SR_BSK2_ROZE 14
#define ITS_PIN_SR_BSK3_ROZE 26
#define ITS_PIN_SR_BSK4_ROZE 30

#define ITS_PIN_SR_BSK1_CMD  11
#define ITS_PIN_SR_BSK2_CMD  15
#define ITS_PIN_SR_BSK3_CMD  27
#define ITS_PIN_SR_BSK4_CMD  31


#endif /* MAIN_PINOUT_CFG_H_ */
