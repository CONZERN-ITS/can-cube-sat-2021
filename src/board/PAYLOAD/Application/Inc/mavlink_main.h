/*
 * mavlink_main.h
 *
 *  Created on: Apr 10, 2021
 *      Author: developer
 */

#ifndef INC_MAVLINK_MAIN_H_
#define INC_MAVLINK_MAIN_H_

#include <stddef.h>

//! Уменьшаем количество буферов мавлинка до двух
#define MAVLINK_COMM_NUM_BUFFERS 2

//! Включаем удобные мавлинковые функции
//#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
// Скидывать сообщения в текстовом виде в консольку для отладки
#ifdef DEBUG
#define PROCESS_TO_PRINTF
#endif
// Скидывать сообщения в its_link
#define PROCESS_TO_ITSLINK

#include <../mavlink_types.h>

//! Определяем идентификаторы системы
extern uint8_t mavlink_system;

//! Отправка mavlink пакета в i2c-link
void mav_main_send_to_its_link(mavlink_channel_t channel, const uint8_t * buffer, uint16_t buffer_size);

//! Получение mavlink пакета и i2c-link
//! Возвращает 0 если пакет есть и ошибку если ошибка (включая EAGAIN)
int mav_main_get_packet_from_its_link(mavlink_message_t * msg);

//! Получение пакета из отладочного уарта
int mav_main_get_packet_from_imitator_ctl(mavlink_message_t * msg);

//! Определяем функцию для отправки телеметрии
#define MAVLINK_SEND_UART_BYTES mav_main_send_to_its_link

// Наконец то подключаем сам мавлинк
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <mavlink.h>
#pragma GCC diagnostic pop

void mav_main_process_bme_message(const mavlink_pld_bme280_data_t * msg, PLD_LOCATION location);
void mav_main_process_ms5611_message(const mavlink_pld_ms5611_data_t * msg, PLD_LOCATION location);
void mav_main_process_me2o2_message(mavlink_pld_me2o2_data_t * msg);
void mav_main_process_mics_message(mavlink_pld_mics_6814_data_t * msg);
void mav_main_process_owntemp_message(mavlink_own_temp_t * msg);
void mav_main_process_dosim_message(mavlink_pld_dosim_data_t * msg);
void mav_main_process_dna_message(mavlink_pld_dna_data_t * msg);
void mav_main_process_own_stats(mavlink_pld_stats_t * msg);
void mav_main_process_i2c_link_stats(mavlink_i2c_link_stats_t * msg);
void mav_main_process_commissar_report(uint8_t component_id, const mavlink_commissar_report_t * report);


#endif /* INC_MAVLINK_MAIN_H_ */
