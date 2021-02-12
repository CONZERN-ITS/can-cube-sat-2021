#include <ccsds/uslp/input_stack.hpp>

#include <cassert>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


static input_stack_event_handler _default_event_handler;


input_stack::input_stack()
{
	set_event_handler(&_default_event_handler);
}


void input_stack::set_event_handler(input_stack_event_handler * event_handler)
{
	_event_handler = event_handler;
}


void input_stack::push_frame(const uint8_t * frame_data, size_t frame_data_size)
{
	if (!_pchannel)
		throw einval_exception("unable to push data to input stack without pchannel in it");

	_pchannel->push_frame(frame_data, frame_data_size);
}


void input_stack::_event_callback(const acceptor_event & evt)
{
	switch (evt.kind)
	{
	case acceptor_event::kind_t::MAP_SDU:
		_event_handler->on_map_sdu_event(dynamic_cast<const acceptor_event_map_sdu&>(evt));
		break;

	default:
		assert(false);
		break;
	}
}


}}
