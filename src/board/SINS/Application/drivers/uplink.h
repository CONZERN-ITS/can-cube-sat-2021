/*
 * uplink.h
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */

#ifndef DRIVERS_UPLINK_H_
#define DRIVERS_UPLINK_H_


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <its/mavlink.h>
#pragma GCC diagnostic pop


int uplink_init(void);

int uplink_write_raw(const void * data, int data_size);

int uplink_write_mav(const mavlink_message_t * msg);


#endif /* DRIVERS_UPLINK_H_ */
