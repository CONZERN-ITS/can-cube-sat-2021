#include <iostream>
#include <ostream>
#include <iomanip>
#include <cstring>
#include <cassert>
#include <fstream>
#include <numeric>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/idle_pattern.hpp>

#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>
#include <ccsds/uslp/virtual/map_rr_muxer.hpp>
#include <ccsds/uslp/map/map_access_source.hpp>
#include <ccsds/uslp/map/map_packet_source.hpp>

#include <ccsds/uslp/physical/mchannel_demuxer.hpp>
#include <ccsds/uslp/master/vchannel_demuxer.hpp>
#include <ccsds/uslp/virtual/map_demuxer.hpp>
#include <ccsds/uslp/map/map_access_sink.hpp>
#include <ccsds/uslp/map/map_packet_sink.hpp>

#include "bytes_printer.hpp"
#include "lorem_ipsum.hpp"


struct downlink_stack_t
{
	downlink_stack_t()
		: phys("downlink"),
		  master(ccsds::uslp::mcid_t(0x100)),
		  virt1(ccsds::uslp::gvcid_t(0x100, 0)),
		  virt2(ccsds::uslp::gvcid_t(0x100, 1)),
		  map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0)),
		  map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1)),
		  map_p3(ccsds::uslp::gmap_id_t(0x100, 1, 2))
	{
		phys.add_mchannel_sink(&master);
		master.add_vchannel_sink(&virt1);
		master.add_vchannel_sink(&virt2);
		virt1.add_map_sink(&map_s1);
		virt1.add_map_sink(&map_s2);
		virt2.add_map_sink(&map_p3);

		map_p3.emit_idle_packets(true);

		phys.finalize();
	}


	void push_frame(const uint8_t * frame_buffer, size_t frame_buffer_size)
	{
		phys.push_frame(frame_buffer, frame_buffer_size);
	}


	typedef std::function<
			void(
					ccsds::uslp::gmap_id_t,
					const ccsds::uslp::map_sink_event & event
			)
	> map_event_handler_t;


	void set_event_handler(map_event_handler_t event_handler)
	{
		ccsds::uslp::map_sink * maps[] = {
				&map_s1, &map_s2, &map_p3
		};

		for (auto * map_sink: maps)
		{
			map_sink->set_event_callback([event_handler, map_sink](const ccsds::uslp::map_sink_event & event){
				event_handler(map_sink->map_id, event);
			});
		}
	}

	ccsds::uslp::mchannel_demuxer phys;
	ccsds::uslp::vchannel_demuxer master;
	ccsds::uslp::map_demuxer virt1;
	ccsds::uslp::map_demuxer virt2;
	ccsds::uslp::map_access_sink map_s1;
	ccsds::uslp::map_access_sink map_s2;
	ccsds::uslp::map_packet_sink map_p3;
};



struct uplink_stack_t
{
	uplink_stack_t(size_t frame_buffer_size)
		: phys("loraloralora"),
		  master(ccsds::uslp::mcid_t(0x100)),
		  virt1(ccsds::uslp::gvcid_t(0x100, 0)),
		  virt2(ccsds::uslp::gvcid_t(0x100, 1)),
		  map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0)),
		  map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1)),
		  map_p3(ccsds::uslp::gmap_id_t(0x100, 1, 2))
	{
		phys.frame_size(frame_buffer_size);
		phys.add_mchannel_source(&master);

		master.add_vchannel_source(&virt1);
		master.add_vchannel_source(&virt2);

		virt1.frame_seq_no_len(2);
		virt1.add_map_source(&map_s1);
		virt1.add_map_source(&map_s2);
		virt2.add_map_source(&map_p3);

		phys.error_control_len(ccsds::uslp::error_control_len_t::ZERO);
		master.id_is_destination(true);

		phys.finalize();
	}


	bool peek_frame()
	{
		return phys.peek();
	}


	bool peek_frame(ccsds::uslp::pchannel_frame_params_t & frame_params)
	{
		return phys.peek(frame_params);
	}


	void pop_frame(uint8_t * frame_buffer, size_t frame_buffer_size)
	{
		return phys.pop(frame_buffer, frame_buffer_size);
	}


	int32_t frame_buffer_size() const { return phys.frame_size(); }


	ccsds::uslp::mchannel_rr_muxer phys;
	ccsds::uslp::vchannel_rr_muxer master;
	ccsds::uslp::map_rr_muxer virt1;
	ccsds::uslp::map_rr_muxer virt2;
	ccsds::uslp::map_access_source map_s1;
	ccsds::uslp::map_access_source map_s2;
	ccsds::uslp::map_packet_source map_p3;
};


void event_handler(ccsds::uslp::gmap_id_t map_id, const ccsds::uslp::map_sink_event & event_)
{
	switch (event_.kind)
	{
	case ccsds::uslp::map_sink_event::event_kind_t::DATA_UNIT: {
			auto & event = dynamic_cast<const ccsds::uslp::map_sink_event_data_unit & >(event_);
			std::cout << "got map sink event "
					<< map_id.sc_id() << ", " << (int)map_id.vchannel_id() << ", " << (int)map_id.map_id() << " "
					<< "sdu (" << event.data.size() << ") "
					<< ": " << print_bytes(event.data) << std::endl;
	} break;

	}; // switch
}


static uint8_t _natural_data[4096];


int main()
{
	std::iota(std::begin(_natural_data), std::end(_natural_data), static_cast<uint8_t>(0));

	const uint8_t idle_pattern[] = {0xCA, 0xDE, 0xBA, 0xBA};
	ccsds::uslp::idle_pattern_generator::instance().assign(
			std::cbegin(idle_pattern),
			std::cend(idle_pattern)
	);

	uint8_t frame_buffer[142];
	uplink_stack_t uplink_stack(sizeof(frame_buffer));
	downlink_stack_t downlink_stack;
	downlink_stack.set_event_handler(event_handler);

	//auto * data = reinterpret_cast<const uint8_t*>(lorem_impsum_1k);
	//const size_t data_size = sizeof(lorem_impsum_1k) - 1; // Без терминирующего ноля плез

	const auto * data = _natural_data;
	const size_t data_size = sizeof(_natural_data);

	uplink_stack.map_p3.add_encapsulated_data(
			data, data_size,
			ccsds::uslp::qos_t::SEQUENCE_CONTROLLED,
			ccsds::epp::protocol_id_t::PRIVATE
	);

	uplink_stack.map_s1.add_sdu(data, data_size, ccsds::uslp::qos_t::SEQUENCE_CONTROLLED);

	std::ofstream file(
			"/tmp/ccsds_lorem_impsum.map_142_two_vc",
			std::ios::trunc | std::ios::out | std::ios::binary
	);

	int i = 0;
	ccsds::uslp::pchannel_frame_params_t frame_params;
	while (uplink_stack.peek_frame(frame_params))
	{
		std::cout << "peek" << i << "!"
				<< frame_params.channel_id.sc_id() << ","
				<< static_cast<int>(frame_params.channel_id.vchannel_id()) << ","
				<< static_cast<int>(frame_params.channel_id.map_id()) << "; "
				<< static_cast<int>(frame_params.frame_class) << "; "
				<< frame_params.frame_seq_no->value() << "@"
					<< static_cast<int>(frame_params.frame_seq_no->value_size()) << "; "
				<< frame_params.id_is_destination << "; "
				<< frame_params.ocf_present << "; "
				<< std::endl;

		std::memset(frame_buffer, 0x00, sizeof(frame_buffer));
		uplink_stack.pop_frame(frame_buffer, sizeof(frame_buffer));
		std::cout << print_bytes(frame_buffer) << std::endl;
		i++;

		file.write(reinterpret_cast<const char*>(frame_buffer), sizeof(frame_buffer));
		downlink_stack.push_frame(frame_buffer, sizeof(frame_buffer));
	}

	return 0;
}
