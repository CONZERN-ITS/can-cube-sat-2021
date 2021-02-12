#ifndef SRC_EVENT_HANDLER_HPP_
#define SRC_EVENT_HANDLER_HPP_


#include <zmq.hpp>

#include <ccsds/uslp/events.hpp>
#include <ccsds/uslp/input_stack.hpp>
#include <ccsds/uslp/output_stack.hpp>


class event_handler:
		public ccsds::uslp::input_stack_event_handler,
		public ccsds::uslp::output_stack_event_handler
{
public:
	event_handler(zmq::socket_t & socket)
	:	_output_socket(socket)
	{}


protected:
	using acceptor_event_map_sdu = ccsds::uslp::acceptor_event_map_sdu;
	using emitter_event_sdu_emitted = ccsds::uslp::emitter_event_sdu_emitted;

	virtual void on_map_sdu_event(const acceptor_event_map_sdu & event) override
	{

	}

	virtual void on_frame_emitted(const emitter_event_sdu_emitted & event) override
	{

	}

private:
	void _send_telemetry_packet(const acceptor_event_map_sdu & event)
	{
		constexpr char topic[] = "uslp.telemetry";
	}

	zmq::socket_t & _output_socket;

};




#endif /* SRC_EVENT_HANDLER_HPP_ */
