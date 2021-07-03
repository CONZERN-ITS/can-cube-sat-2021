/*
 * send.h
 *
 *  Created on: Jul 24, 2020
 *      Author: sereshotes
 */

#ifndef COMPONENTS_RADIO_INC_RADIO_H_
#define COMPONENTS_RADIO_INC_RADIO_H_


#include "sx126x_drv.h"
#include "mavlink_help2.h"

// Периоды отправки сообщений. Больше - реже
#define RADIO_SEND_ID_ARRAY(F) \
	F(0, MAVLINK_MSG_ID_ELECTRICAL_STATE, 15) \
	F(1, MAVLINK_MSG_ID_OWN_TEMP, 20) \
	F(2, MAVLINK_MSG_ID_THERMAL_STATE, 100) \
	F(3, MAVLINK_MSG_ID_PLD_BME280_DATA, 15) \
	F(4, MAVLINK_MSG_ID_PLD_ME2O2_DATA, 15) \
	F(5, MAVLINK_MSG_ID_PLD_MICS_6814_DATA, 15) \
	F(6, MAVLINK_MSG_ID_SINS_isc, 5) \
	F(7, MAVLINK_MSG_ID_GPS_UBX_NAV_SOL, 15)

#define RADIO_SEND_BAN(F) \
	F(MAVLINK_MSG_ID_TIMESTAMP)

#define RADIO_DEFAULT_PERIOD 30
#define RADIO_SLEEP_DEFAULT 2000



#define RADIO_PACKET_SIZE ITS_RADIO_PACKET_SIZE
#define RADIO_RX_TIMEDOUT_LIMIT 5
#define RADIO_TX_TIMEOUT_MS (10000)
#define RADIO_RX_TIMEOUT_MS (5000)
#define RADIO_RSSI_PERIOD_MS (500)
#define SERVER_TX_STATE_PERIOD_MS (500)
#define SERVER_POLL_TIMEOUT_MS (1000)
#define SERVER_SMALL_TIMEOUT_MS 4000

typedef uint64_t msg_cookie_t;
#define MSG_COOKIE_T_PLSHOLDER PRIu64


typedef struct {
	size_t size;
	size_t index;
	size_t capacity;
	uint8_t buf[];
} radio_buf_t;

typedef struct {
	uint16_t count_recieved_mavlink_msgs;
	uint32_t last_packet_time;
} radio_stats_t;

typedef struct radio_t {
	sx126x_drv_t dev;
	radio_buf_t mav_buf;
	uint8_t _mav_buf[MAVLINK_MAX_PACKET_LEN];

	radio_buf_t radio_buf;
	uint8_t _radio_buf[ITS_RADIO_PACKET_SIZE];

	uint8_t mavlink_chan;
	uint32_t msg_count;
	uint16_t packet_count;
	int is_ready_to_send;

	radio_stats_t stats;
} radio_t;


/*
 * Инициализирует отправку сообщений по радио с заданными через
 * RADIO_SEND_ID_ARRAY, RADIO_SEND_BAN, RADIO_DEFAULT_PERIOD их частотами.
 */
void radio_send_init(void);

void radio_send_suspend(void);

void radio_send_resume(void);

void radio_send_set_sleep_delay(int64_t sleep_delay);


#endif /* COMPONENTS_RADIO_INC_RADIO_H_ */
