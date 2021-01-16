#include <ccsds/uslp/master/abstract_mchannel.hpp>

namespace ccsds { namespace uslp {


mchannel_source::mchannel_source(mcid_t mcid_)
	: mcid(mcid_)
{

}


mchannel_source::mchannel_source(mcid_t mcid_, uint16_t frame_size_l1_)
	: mcid(mcid_), _frame_size_l1(frame_size_l1_)
{

}


void mchannel_source::frame_size_l1(uint16_t value)
{
	_frame_size_l1 = value;
}


void mchannel_source::id_is_destination(bool value)
{
	_id_is_destination = value;
}


}}
