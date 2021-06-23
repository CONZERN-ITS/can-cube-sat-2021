#ifndef SERVER_RADIO_ZMQ_H_
#define SERVER_RADIO_ZMQ_H_


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#include <sx126x_defs.h>


typedef uint64_t msg_cookie_t;
#define MSG_COOKIE_T_PLSHOLDER PRIu64


typedef struct zserver_t
{
	void * zmq;
	void * sub_socket;
	void * pub_socket;
} zserver_t;


struct server_stats_t;
typedef struct server_stats_t server_stats_t;


int zserver_init(zserver_t * zserver);

void zserver_deinit(zserver_t * zserver);


int zserver_recv_tx_packet(
	zserver_t * zserver, uint8_t * buffer, size_t buffer_size,
	size_t * packet_size, msg_cookie_t * packet_cookie
);


int zserver_send_tx_buffers_state(
	zserver_t * zserver,
	msg_cookie_t cookie_wait, msg_cookie_t cookie_in_progress,
	msg_cookie_t cookie_sent, msg_cookie_t cookie_dropped
);


int zserver_send_packet_rssi(
	zserver_t * zserver,
	msg_cookie_t packet_cookie,
	int8_t rssi_pkt, int8_t snr_pkt, int8_t signal_rssi_pkt
);


int zserver_send_rx_packet(
	zserver_t * zserver,
	const uint8_t * packet_data, size_t packet_data_size,
	msg_cookie_t packet_cookie, const uint16_t * packet_no,
	bool crc_valid,	int8_t rssi_pkt, int8_t snr_pkt, int8_t signal_rssi_pkt
);


int zserver_send_stats(
	zserver_t * zserver, const sx126x_stats_t * stats, uint16_t device_errors,
	const server_stats_t * server_stats
);

int zserver_send_instant_rssi(zserver_t * zserver, int8_t rssi);


#endif // SERVER_RADIO_ZMQ_H_
