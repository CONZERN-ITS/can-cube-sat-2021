/*/
 * uplink.c
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */

#include "uplink.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>


#include <its-i2c-link.h>

#include "common.h"
#include "commissar.h"

//! Буфер под инжектируемые сообщения
typedef struct its_injected_message_t
{
	// Идентификатор сообщения
	const uint32_t msgid;
	//! Пуст буфер или полон
	bool have_data;
	//! Сам сообщение
	uint8_t message[MAVLINK_MAX_PACKET_LEN];
	//! Длина сообщения
	uint8_t len;
} its_injected_message_t;


//! Контейнер буферов под инжектируемые сообщения
static its_injected_message_t _injected_msgs[] =
{
		{ .msgid = MAVLINK_MSG_ID_TIMESTAMP },
		{ .msgid = MAVLINK_MSG_ID_GPS_UBX_NAV_SOL },
		{ .msgid = MAVLINK_MSG_ID_SINS_errors },
		{ .msgid = MAVLINK_MSG_ID_I2C_LINK_STATS },
		{ .msgid = MAVLINK_MSG_ID_OWN_TEMP },
};


//! Сохраняет сообщение, если оно иженктируемое и под него есть место
/*! Возвращает true если сообщение было сохранено */
static bool _store_message_if_injected(const uint8_t * msg, uint32_t msgid, uint8_t len)
{
	for (size_t i = 0; i < sizeof(_injected_msgs)/sizeof(_injected_msgs[0]); i++)
	{
		its_injected_message_t * record = &_injected_msgs[i];
		if (record->msgid == msgid)
		{
			// У нас есть ячейка под такие сообщения
			memcpy(record->message, msg, len);
			record->have_data = true;
			record->len = len;
			return true;
		}
	}

	return false;
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
			int error = its_i2c_link_write(record->message, record->len);
			//printf("msgid %d\n", (int)record->msgid);
			if (0 > error)
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
//	error = uplink_flush_injected();
//	printf("error %d\n", error);
	if (error == -EAGAIN)
	{
		// Ну если мы получили такую ошибку
		// то нам и пытаться не стоит
		return -EAGAIN;
	}


	// После инжектируемых пробуем себя
	uint8_t msg_buffer[MAVLINK_MAX_PACKET_LEN];

	uint16_t len = mavlink_msg_to_send_buffer(msg_buffer, msg);
	error = its_i2c_link_write(msg_buffer, len);
	// > 0 это отсутствие ошибки для комиссара
	commissar_report(COMMISSAR_SUB_I2C_LINK, error > 0 ? 0 : error);

	//printf("msgid %d\n", msg->msgid);
	if (error == -EAGAIN)
	{
		// Если не удалось отправить сообщение
		// потому что буфер переполнен
		// откладываем сообщение на потом
		_store_message_if_injected(msg_buffer, msg->msgid, len);
	}

	#ifdef _IWDG
	if (0 == error)
	{
		// Перезапускаем iwdg после успешной отпраки сообщения
		HAL_IWDG_Refresh(&hiwdg);
	}
	#endif

	// its_i2c_link_write возвращает количество байт в случае успеха. Нам тут это не надо
	// Сводим ответ до 0 в случае успеха

	if (error)
		return error;

	return 0;
}

