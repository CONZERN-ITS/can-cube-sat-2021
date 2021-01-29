#include <ccsds/uslp/output_stack.hpp>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


output_stack::output_stack()
{

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


}}
