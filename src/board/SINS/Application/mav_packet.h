/*
 * mav_packet.h
 *
 *  Created on: 22 апр. 2020 г.
 *      Author: developer
 */

#ifndef MAV_PACKET_H_
#define MAV_PACKET_H_

#include "state.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <its/mavlink.h>
#pragma GCC diagnostic pop


int mavlink_sins_isc(stateSINS_isc_t * state_isc);
int mavlink_timestamp(void);
void on_gps_packet(void * arg, const ubx_any_packet_t * packet);
int own_temp_packet(void);
int mavlink_errors_packet(void);

#endif /* MAV_PACKET_H_ */
