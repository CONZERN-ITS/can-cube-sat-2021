/*
 * uplink.h
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */

#ifndef DRIVERS_UPLINK_H_
#define DRIVERS_UPLINK_H_

#include <stm32f4xx_hal.h>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <its/mavlink.h>
#pragma GCC diagnostic pop


extern IWDG_HandleTypeDef hiwdg;


//! Инициализация
int uplink_init(void);

//! Повторные попытки отправки инжектируемых сообщений
int uplink_flush_injected();

int uplink_write_mav(const mavlink_message_t * msg);


#endif /* DRIVERS_UPLINK_H_ */
