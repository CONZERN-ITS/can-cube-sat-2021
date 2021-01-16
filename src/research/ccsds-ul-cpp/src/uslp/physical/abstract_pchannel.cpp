#include <ccsds/uslp/physical/abstract_pchannel.hpp>


namespace ccsds { namespace uslp {


pchannel_source::pchannel_source(std::string name_)
		: name(name_)
{

}


pchannel_source::pchannel_source(std::string name_, int32_t frame_size_)
	: name(name_)
{
	frame_size(frame_size_);
}


pchannel_source::pchannel_source(std::string name_, int32_t frame_size_, error_control_len_t err_control_len_)
	: name(name_)
{
	frame_size(frame_size_);
	error_control_len(err_control_len_);
}


void pchannel_source::frame_size(int32_t value)
{
	// TODO: Проверку на валидность значения
	_frame_size = value;
}


void pchannel_source::error_control_len(error_control_len_t value)
{
	_error_control_len = value;
}


}}

