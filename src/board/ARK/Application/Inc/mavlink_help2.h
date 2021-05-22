/*
 * mavlink_help2.h
 *
 *  Created on: Jul 16, 2020
 *      Author: sereshotes
 */

#ifndef INC_MAVLINK_HELP2_H_
#define INC_MAVLINK_HELP2_H_


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include "mavlink.h"

#include "assert.h"

#if defined CUBE_1
const static uint8_t mavlink_system = CUBE_1_PCU;
#elif defined CUBE_2
const static uint8_t mavlink_system = CUBE_2_PCU;
#endif

mavlink_channel_t mavlink_claim_channel(void);

#endif /* INC_MAVLINK_HELP2_H_ */
