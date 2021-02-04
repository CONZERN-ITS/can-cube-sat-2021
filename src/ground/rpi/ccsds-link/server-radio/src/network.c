#include "network.h"

#include <string.h>

#include "zmq.h"


int network_init(network_t * network)
{
	memset(network, 0x00, sizeof(network));

	network->zmq_ctx = zmq_ctx_new();
	if (!network->zmq_ctx)
		return -1;

	network->zmq_data_socket = zmq_socket(network->zmq_ctx, ZMQ_REQ);
	if (!network->zmq_data_socket)
		return -1;

	int rc = zmq_bind(network->zmq_data_socket, "tcp://*:5555");
	if (0 != rc)
		return rc;

	return 0;
}


int network_push_data_packet(network_t * network, const uint8_t * data, size_t data_size)
{
	int rc = zmq_send(network->zmq_data_socket, data, data_size, ZMQ_DONTWAIT);

	//zmq_recv(network->zmq_data_socket, buf_, len_, flags_), s_, flags_)

	return 0;
}


void network_deinit(network_t * network)
{
	if (NULL != network->zmq_data_socket)
	{
		zmq_close(network->zmq_data_socket);
		network->zmq_data_socket = NULL;
	}

	if (NULL != network->zmq_ctx)
	{
		zmq_ctx_destroy(network->zmq_ctx);
		network->zmq_ctx = NULL;
	}
}
