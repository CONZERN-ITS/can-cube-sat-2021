#include <ccsds/uslp/master/abstract_mchannel.hpp>

namespace ccsds { namespace uslp {


	void mchannel_service::max_frame_size(uint16_t value)
	{
		_max_frame_size = value;
	}


}}
