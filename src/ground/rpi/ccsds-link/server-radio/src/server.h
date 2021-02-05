#ifndef SERVER_RADIO_SRC_RADIO_CONFIG_H_
#define SERVER_RADIO_SRC_RADIO_CONFIG_H_

#include "sx126x_drv.h"

#define RADIO_PACKET_SIZE 200
#define RADIO_RX_TIMEDOUT_LIMIT 5
#define RADIO_TX_TIMEOUT_MS (5000)
#define RADIO_RX_TIMEOUT_MS (5000)
#define RADIO_RSSI_PERIOD_MS (500)
#define SERVER_POLL_TIMEOUT_MS (500)

#define SERVER_DATA_SOCKET_EP "tcp://0.0.0.0:5050"
#define SERVER_TELEMETRY_SOCKET_EP "tcp://0.0.0.0:5051"

typedef uint16_t msg_cookie_t;


typedef struct server_t
{
	sx126x_drv_t dev;

	uint8_t rx_buffer[255];
	size_t rx_buffer_capacity;
	size_t rx_buffer_size;
	uint32_t rx_timeout_ms;
	size_t rx_timedout_cnt;
	size_t rx_timedout_limit;

	uint8_t tx_buffer[255];
	size_t tx_buffer_capacity;
	size_t tx_buffer_size;
	msg_cookie_t tx_cookie_wait;
	msg_cookie_t tx_cookie_in_progress;
	msg_cookie_t tx_cookie_sent;
	msg_cookie_t tx_cookie_dropped;
	uint32_t tx_timeout_ms;

	void * zmq;
	void * data_socket;
	void * telemetry_socket;
} server_t;


int server_init(server_t * server);

void server_run(server_t * server);

void server_deinit(server_t * server);


#endif /* SERVER_RADIO_SRC_RADIO_CONFIG_H_ */
