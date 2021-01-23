#include <iostream>
#include <ostream>
#include <iomanip>
#include <cstring>
#include <cassert>

#include <ccsds/uslp/ids.hpp>
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


#include <bytes_printer.hpp>


struct downlink_stack_t
{
	downlink_stack_t()
		: phys("downlink"),
		  master(ccsds::uslp::mcid_t(0x100)),
		  virt(ccsds::uslp::gvcid_t(0x100, 0)),
		  map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0)),
		  map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1)),
		  map_p3(ccsds::uslp::gmap_id_t(0x100, 0, 3))
	{
		phys.add_mchannel_sink(&master);
		master.add_vchannel_sink(&virt);
		virt.add_map_sink(&map_s1);
		virt.add_map_sink(&map_s2);
		virt.add_map_sink(&map_p3);

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
			map_s1.set_event_callback([event_handler, map_sink](const ccsds::uslp::map_sink_event & event){
				event_handler(map_sink->map_id, event);
			});
		}
	}

	ccsds::uslp::mchannel_demuxer phys;
	ccsds::uslp::vchannel_demuxer master;
	ccsds::uslp::map_demuxer virt;
	ccsds::uslp::map_access_sink map_s1;
	ccsds::uslp::map_access_sink map_s2;
	ccsds::uslp::map_packet_sink map_p3;
};



struct uplink_stack_t
{
	uplink_stack_t(size_t frame_buffer_size)
		: phys("loraloralora"),
		  master(ccsds::uslp::mcid_t(0x100)),
		  virt(ccsds::uslp::gvcid_t(0x100, 0)),
		  map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0)),
		  map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1)),
		  map_p3(ccsds::uslp::gmap_id_t(0x100, 0, 2))
	{
		phys.frame_size(frame_buffer_size);
		phys.error_control_len(ccsds::uslp::error_control_len_t::ZERO);
		phys.add_mchannel_source(&master);

		master.add_vchannel_source(&virt);

		virt.frame_seq_no_len(2);
		virt.add_map_source(&map_s1);
		virt.add_map_source(&map_s2);
		virt.add_map_source(&map_p3);

		phys.finalize();
	}


	bool peek_frame()
	{
		return phys.peek_frame();
	}


	bool peek_frame(ccsds::uslp::pchannel_frame_params_t & frame_params)
	{
		return phys.peek_frame(frame_params);
	}


	void pop_frame(uint8_t * frame_buffer)
	{
		return phys.pop_frame(frame_buffer);
	}


	int32_t frame_buffer_size() const { return phys.frame_size(); }


	ccsds::uslp::mchannel_rr_muxer phys;
	ccsds::uslp::vchannel_rr_muxer master;
	ccsds::uslp::map_rr_muxer virt;
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
			std::cout << "got mapa "
					<< map_id.sc_id() << ", " << map_id.vchannel_id() << ", " << (int)map_id.map_id() << " "
					<< "sdu (" << event.data.size() << ") "
					<< ": " << print_bytes(event.data) << std::endl;
	} break;

	}; // switch
}


int main()
{
	uint8_t frame_buffer[142];
	uplink_stack_t uplink_stack(sizeof(frame_buffer));
	downlink_stack_t downlink_stack;
	downlink_stack.set_event_handler(event_handler);


	const char data_sample[] = { '\xDE', '\xAD', '\xBE', '\xAF' };
	uint8_t data[200];
	for (size_t i = 0; i < sizeof(data); i++)
		data[i] = data_sample[i % sizeof(data_sample)];

	//uplink_stack.map_s1.add_sdu(data, sizeof(data), ccsds::uslp::qos_t::SEQUENCE_CONTROLLED);
	uplink_stack.map_p3.add_encapsulated_data(
			data, sizeof(data),
			ccsds::uslp::qos_t::SEQUENCE_CONTROLLED,
			ccsds::epp::protocol_id_t::PRIVATE
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
				<< frame_params.ocf_present << "; ";

		std::memset(frame_buffer, 0, sizeof(frame_buffer));
		uplink_stack.pop_frame(frame_buffer);
		std::cout << print_bytes(frame_buffer) << std::endl;
		i++;

		downlink_stack.push_frame(frame_buffer, sizeof(frame_buffer));
	}


	return 0;
}
