#include <ccsds/uslp/output_stack.hpp>

#include <cassert>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


static output_stack_event_handler _default_event_handler;


output_stack::output_stack()
{
	set_event_handler(&_default_event_handler);
}


bool output_stack::peek_frame()
{
	if (!_pchannel)
		throw einval_exception("unable to peek output stack frame without pchannel in it");

	return _pchannel->peek();
}


bool output_stack::peek_frame(pchannel_frame_params_t & frame_params)
{
	if (!_pchannel)
		throw einval_exception("unable to peek output stack frame without pchannel in it");

	return _pchannel->peek(frame_params);
}


void output_stack::pop_frame(uint8_t * frame_buffer, size_t frame_buffer_size)
{
	if (!_pchannel)
		throw einval_exception("unable to pop output stack frame without pchannel in it");

	_pchannel->pop(frame_buffer, frame_buffer_size);
}


void output_stack::_event_callback(const emitter_event & evt)
{
	switch(evt.kind)
	{
	case emitter_event::kind_t::SDU_EMITTED:
		_event_handler->on_frame_emitted(dynamic_cast<const emitter_event_sdu_emitted&>(evt));
		break;

	default:
		assert(false);
	}
}

}}
