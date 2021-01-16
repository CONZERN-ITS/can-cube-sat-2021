#include <ccsds/uslp/master/abstract_mchannel.hpp>

namespace ccsds { namespace uslp {

mchannel_source::mchannel_source(mcid_t mcid_)
	: mcid(mcid_)
{

}


mchannel_source::mchannel_source(mcid_t mcid_, uint16_t frame_data_unit_size_)
	: mcid(mcid_)
{
	frame_data_unit_size(frame_data_unit_size_);
}


void mchannel_source::frame_data_unit_size(uint16_t value)
{
	_frame_unit_size = value;
}


void mchannel_source::id_is_destination(bool value)
{
	_id_is_destination = value;
}


}}
