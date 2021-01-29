#include <iostream>
#include <ostream>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <fstream>
#include <numeric>

#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/events.hpp>
#include <ccsds/uslp/idle_pattern.hpp>

#include <ccsds/uslp/output_stack.hpp>
#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>
#include <ccsds/uslp/virtual/map_rr_muxer.hpp>
#include <ccsds/uslp/map/map_packet_source.hpp>


#include <ccsds/uslp/input_stack.hpp>
#include <ccsds/uslp/physical/mchannel_demuxer.hpp>
#include <ccsds/uslp/master/vchannel_demuxer.hpp>
#include <ccsds/uslp/virtual/map_demuxer.hpp>
#include <ccsds/uslp/map/map_packet_sink.hpp>


#include "bytes_printer.hpp"
#include "lorem_ipsum.hpp"


class console_event_handler: public ccsds::uslp::input_stack_event_handler
{
public:
	virtual void on_map_sdu_event(const ccsds::uslp::gmapid_t gmapid, const ccsds::uslp::map_sdu_event & evt)
	{
		std::cout << "got map sdu event: " << gmapid << ", " << print_bytes(evt.data) << std::endl;
	}
};



int main()
{
	uint8_t frame_buffer[142];

	ccsds::uslp::output_stack ostack;
	auto * opchannel = ostack.create_pchannel<ccsds::uslp::mchannel_rr_muxer>(std::string("olora"));
	opchannel->frame_size(sizeof(frame_buffer));
	opchannel->error_control_len(ccsds::uslp::error_control_len_t::ZERO);

	auto * omchannel = ostack.create_mchannel<ccsds::uslp::vchannel_rr_muxer>(ccsds::uslp::mcid_t(0x100));
	omchannel->id_is_destination(true);

	auto * ovchannel = ostack.create_vchannel<ccsds::uslp::map_rr_muxer>(ccsds::uslp::gvcid_t(0x100, 0x00));
	ovchannel->frame_seq_no_len(2);

	auto * ompchannel = ostack.create_map<ccsds::uslp::map_packet_source>(ccsds::uslp::gmapid_t(0x100, 0x00, 0x00));

	opchannel->finalize();


	ccsds::uslp::input_stack istack;
	auto * ipchannel = istack.create_pchannel<ccsds::uslp::mchannel_demuxer>(std::string("ilora"));
	ipchannel->error_control_len(opchannel->error_control_len());
	ipchannel->insert_zone_size(0);

	auto * imchannel = istack.create_mchannel<ccsds::uslp::vchannel_demuxer>(ccsds::uslp::mcid_t(0x100));

	auto * ivchannel = istack.create_vchannel<ccsds::uslp::map_demuxer>(ccsds::uslp::gvcid_t(0x100, 0x00));

	auto * impchannel = istack.create_map<ccsds::uslp::map_packet_sink>(ccsds::uslp::gmapid_t(0x100, 0x00, 0x00));

	console_event_handler evt_handler;
	istack.set_event_handler(&evt_handler);

	ipchannel->finalize();



	uint8_t packet_data[4096];
	for (size_t i = 0; i < sizeof(packet_data); i++)
		packet_data[i] = i & 0xFF;

	ompchannel->add_encapsulate_data(
			packet_data, sizeof(packet_data),
			ccsds::uslp::qos_t::EXPIDITED,
			ccsds::epp::protocol_id_t::PRIVATE
	);

	while(ostack.peek_frame())
	{
		ostack.pop_frame(frame_buffer, sizeof(frame_buffer));
		istack.push_frame(frame_buffer, sizeof(frame_buffer));
	}

	return 0;
}
