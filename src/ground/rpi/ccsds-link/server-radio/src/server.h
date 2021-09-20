#ifndef SERVER_RADIO_SRC_RADIO_CONFIG_H_
#define SERVER_RADIO_SRC_RADIO_CONFIG_H_

#include <signal.h>

#include <time.h>
#include <stdbool.h>
#include <inttypes.h>

#include "sx126x_drv.h"

#include "server-zmq.h"
#include "server-config.h"


//! Максимальный размер пакета sx126x. Больше оно просто не может
#define SERVER_MAX_PACKET_SIZE (255)


typedef struct server_stats_t
{
	uint32_t rx_done_counter;
	uint32_t rx_frame_counter;
	uint32_t tx_frame_counter;

	int8_t last_rx_rssi_pkt;
	int8_t last_rx_rssi_signal;
	int8_t last_rx_snr;
	int8_t current_pa_power;
	int8_t requested_pa_power;
} server_stats_t;


typedef struct server_t
{
	server_config_t config;
	sx126x_drv_t radio;
	zserver_t zserver;

	server_stats_t stats;

	struct timespec rx_packet_received;
	size_t rx_timeout_count;
	msg_cookie_t rx_cookie;

	uint8_t tx_buffer[SERVER_MAX_PACKET_SIZE];
	size_t tx_buffer_size;
	msg_cookie_t tx_cookie_wait;
	msg_cookie_t tx_cookie_in_progress;
	msg_cookie_t tx_cookie_sent;
	msg_cookie_t tx_cookie_dropped;
	bool tx_cookies_updated;

	uint16_t radio_errors;

	struct timespec rssi_last_report_timepoint;
	struct timespec radio_stats_last_report_timepoint;
	struct timespec tx_state_last_report_timepoint;

	volatile sig_atomic_t stop_requested;

	int8_t pa_request;

} server_t;


int server_ctor(server_t * server, const server_config_t * config);

void server_dtor(server_t * server);

int server_serve(server_t * server);

int server_request_stop(server_t * server);


#endif /* SERVER_RADIO_SRC_RADIO_CONFIG_H_ */
