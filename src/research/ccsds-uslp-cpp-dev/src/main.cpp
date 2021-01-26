#include <iostream>
#include <ostream>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <fstream>
#include <numeric>

#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/idle_pattern.hpp>

#include <ccsds/uslp/output_stack.hpp>
#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>
#include <ccsds/uslp/virtual/map_rr_muxer.hpp>
#include <ccsds/uslp/map/map_packet_source.hpp>


#include "bytes_printer.hpp"
#include "lorem_ipsum.hpp"



int main()
{
	ccsds::uslp::output_stack ostack;

	ostack.create_pchannel<ccsds::uslp::mchannel_rr_muxer>(std::string("olora"));
	ostack.create_mchannel<ccsds::uslp::vchannel_rr_muxer>(ccsds::uslp::mcid_t(0x100));
	ostack.create_vchannel<ccsds::uslp::map_rr_muxer>(ccsds::uslp::gvcid_t(0x100, 0x00));
	auto * mapp1 = ostack.create_map<ccsds::uslp::map_packet_source>(
			ccsds::uslp::gmapid_t(0x100, 0x00, 0x00)
	);

	return 0;
}
