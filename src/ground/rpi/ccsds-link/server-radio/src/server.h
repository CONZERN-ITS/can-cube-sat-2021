#ifndef SERVER_RADIO_SRC_RADIO_CONFIG_H_
#define SERVER_RADIO_SRC_RADIO_CONFIG_H_

#include <time.h>
#include <stdbool.h>
#include <inttypes.h>

#include "sx126x_drv.h"


#define RADIO_PACKET_SIZE 200
#define RADIO_RX_TIMEDOUT_LIMIT 5
#define RADIO_TX_TIMEOUT_MS (5000)
#define RADIO_RX_TIMEOUT_MS (5000)
#define RADIO_RSSI_PERIOD_MS (500)
#define RADIO_WATCHDOG_TIMEOUT_MS (5000)
#define SERVER_TX_STATE_PERIOD_MS (500)
#define SERVER_POLL_TIMEOUT_MS (1000)

typedef uint64_t msg_cookie_t;
#define MSG_COOKIE_T_PLSHOLDER PRIu64


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

	struct timespec radio_watchdog_start_time;
	struct timespec radio_last_poll_time;

	void * zmq;
	void * sub_socket;
	void * pub_socket;
} server_t;


int server_init(server_t * server);

void server_run(server_t * server);

void server_deinit(server_t * server);


#endif /* SERVER_RADIO_SRC_RADIO_CONFIG_H_ */
