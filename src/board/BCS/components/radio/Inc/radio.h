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
#define RADIO_SEND_ID_ARRAY(F) \
	F(0, MAVLINK_MSG_ID_ELECTRICAL_STATE, 15) \
	F(1, MAVLINK_MSG_ID_OWN_TEMP, 20) \
	F(2, MAVLINK_MSG_ID_THERMAL_STATE, 15) \
	F(3, MAVLINK_MSG_ID_PLD_BME280_DATA, 15) \
	F(4, MAVLINK_MSG_ID_PLD_ME2O2_DATA, 15) \
	F(5, MAVLINK_MSG_ID_PLD_MICS_6814_DATA, 15) \
	F(6, MAVLINK_MSG_ID_SINS_isc, 15) \
	F(7, MAVLINK_MSG_ID_GPS_UBX_NAV_SOL, 5)

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


#define log_error(fmt, ...) ESP_LOGE("radio", fmt, ##__VA_ARGS__)
#define log_trace(fmt, ...) ESP_LOGD("radio", fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) ESP_LOGW("radio", fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) ESP_LOGI("radio", fmt, ##__VA_ARGS__)



typedef struct server_t
{
	sx126x_drv_t dev;

	uint8_t rx_buffer[255];
	size_t rx_buffer_capacity;
	size_t rx_buffer_size;
	msg_cookie_t rx_buffer_cookie;
	bool rx_crc_valid;
	int8_t rx_rssi_pkt;
	int8_t rx_snr_pkt;
	int8_t rx_signal_rssi_pkt;
	uint32_t rx_timeout_ms;
	size_t rx_timedout_cnt;
	size_t rx_timedout_limit;

	uint32_t rssi_report_period;
	struct timespec rssi_report_block_deadline;

	uint8_t tx_buffer[255];
	size_t tx_buffer_capacity;
	size_t tx_buffer_size;
	msg_cookie_t tx_cookie_wait;
	msg_cookie_t tx_cookie_in_progress;
	msg_cookie_t tx_cookie_sent;
	msg_cookie_t tx_cookie_dropped;
	bool tx_cookies_updated;
	uint32_t tx_timeout_ms;

	uint32_t tx_state_report_period_ms;
	struct timespec tx_state_report_block_deadline;

	uint8_t mavlink_chan;

} server_t;



/*
 * Инициализирует отправку сообщений по радио с заданными через
 * RADIO_SEND_ID_ARRAY, RADIO_SEND_BAN, RADIO_DEFAULT_PERIOD их частотами.
 */
void radio_send_init(void);

void radio_send_suspend(void);

void radio_send_resume(void);

void radio_send_set_sleep_delay(int64_t sleep_delay);


void server_start(server_t *server);

int server_init(server_t * server);

void server_task(void *arg);

#endif /* COMPONENTS_RADIO_INC_RADIO_H_ */
