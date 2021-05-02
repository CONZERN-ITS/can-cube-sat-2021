/*/
 * uplink.c
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */

#include "uplink.h"

#include <errno.h>
#include <stdbool.h>

#include <stm32f4xx_hal.h>
#include <its-i2c-link.h>

#include "common.h"

//! Буфер под инжектируемые сообщения
typedef struct its_injected_message_t
{
	// Идентификатор сообщения
	const uint16_t msgid;
	//! Пуст буфер или полон
	bool have_data;
	//! Сам сообщение
	mavlink_message_t message;
} its_injected_message_t;


//! Контейнер буферов под инжектируемые сообщения
static its_injected_message_t _injected_msgs[] =
{
		{ .msgid = MAVLINK_MSG_ID_TIMESTAMP },
		{ .msgid = MAVLINK_MSG_ID_GPS_UBX_NAV_SOL },
		{ .msgid = MAVLINK_MSG_ID_SINS_errors },
		{ .msgid = MAVLINK_MSG_ID_I2C_LINK_STATS },
		{ .msgid = MAVLINK_MSG_ID_OWN_TEMP }
};


//! Сохраняет сообщение, если оно иженктируемое и под него есть место
/*! Возвращает true если сообщение было сохранено */
static bool _store_message_if_injected(const mavlink_message_t * msg)
{
	const uint32_t msgid = msg->msgid;

	for (size_t i = 0; i < sizeof(_injected_msgs)/sizeof(*_injected_msgs); i++)
	{
		its_injected_message_t * record = &_injected_msgs[i];
		if (record->msgid == msgid && !record->have_data)
		{
			// У нас есть ячейка под такие сообщения
			record->message = *msg;
			record->have_data = true;
			return true;
		}
	}

	return false;
}


//! Отправка пакета наружу
static int _do_write_mav(const mavlink_message_t * msg)
{
	static uint8_t msg_buffer[MAVLINK_MAX_PACKET_LEN];

	uint16_t len = mavlink_msg_to_send_buffer(msg_buffer, msg);
	int error = its_i2c_link_write(msg_buffer, len);
	if (error < 0)
		return error;

	// FIXME: Возможно iwdg стоит перезапускать при успешной отправке сообщения?
	// Например добавить if (0 == error)
	//iwdg_reload(&transfer_uart_iwdg_handle);

	// its_i2c_link_write возвращает количество байт в случае успеха. Нам тут это не надо
	// Сводим ответ до 0 в случае успеха
	return 0;
}


int uplink_init()
{
	int error = its_i2c_link_start();
	return error;
}


int uplink_flush_injected()
{
	for (size_t i = 0; i < sizeof(_injected_msgs)/sizeof(*_injected_msgs); i++)
	{
		its_injected_message_t * record = &_injected_msgs[i];
		if (record->have_data)
		{
			// О, такое сообщение у нас есть
			int error = _do_write_mav(&record->message);
			if (0 != error)
				return error;

			// Если оно отправилось
			// Сбрасываем из буфера
			record->have_data = false;

			// Тут нет брейка. будем флашить инжектируемые сообщения пока получается
		}
	}

	return 0;
}


int uplink_write_mav(const mavlink_message_t * msg)
{
	int error = 0;

	// Сперва флашим инжектируемые
	error = uplink_flush_injected();
	if (error == -EAGAIN)
	{
		// Ну если мы получили такую ошибку
		// то нам и пытаться не стоит
		return -EAGAIN;
	}

	// После инжектируемых пробуем себя
	error = _do_write_mav(msg);
	if (error == -EAGAIN)
	{
		// Если не удалось отправить сообщение
		// потому что буфер переполнен
		// откладываем сообщение на потом
		_store_message_if_injected(msg);
	}

	if (error)
		return error;

	return 0;
}

