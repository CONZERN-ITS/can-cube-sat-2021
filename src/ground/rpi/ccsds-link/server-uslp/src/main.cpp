#include <zmq.hpp>

#include "json.hpp"



int main()
{
	zmq::context_t ctx;
	zmq::socket_t socket(ctx, ZMQ_PAIR);


	return 0;
}
