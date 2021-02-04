#ifndef SERVER_RADIO_SRC_NETWORK_H_
#define SERVER_RADIO_SRC_NETWORK_H_


#include <stdint.h>
#include <stddef.h>


typedef struct network_t
{
	void * zmq_ctx;
	void * zmq_data_socket;
} network_t;


int network_init(network_t * network);

int network_push_data_packet(network_t * network, const uint8_t * data, size_t data_size);

void network_deinit(network_t * network);


#endif /* SERVER_RADIO_SRC_NETWORK_H_ */
