/*/
 * uplink.c
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */

#include "uplink.h"

#include <stm32f4xx_hal.h>
#include <its-i2c-link.h>

#include "common.h"

int uplink_init()
{
	int error = its_i2c_link_start();
	return error;
}

int uplink_write_raw(const void * data, int data_size)
{
	int error = its_i2c_link_write(data, data_size);
	return error;
}


int uplink_write_mav(const mavlink_message_t * msg)
{
	// https://mavlink.io/en/about/overview.html#mavlink-2-packet-format
	static uint8_t msg_buffer[280]; // 280 максимальный размер MAV пакета версии 2

	uint16_t len = mavlink_msg_to_send_buffer(msg_buffer, msg);
	int error = uplink_write_raw(msg_buffer, len);
	if (error)
		return error;

	//iwdg_reload(&transfer_uart_iwdg_handle);
	return 0;
}

