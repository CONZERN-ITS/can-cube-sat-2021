#include <iostream>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/physical/mchannel_muxer_pchannel_source.hpp>
#include <ccsds/uslp/master/vchannel_muxer_mchannel_source.hpp>
#include <ccsds/uslp/virtual/map_muxer_vchannel_source.hpp>
#include <ccsds/uslp/map/map_access_source.hpp>
#include <ccsds/uslp/map/map_packet_source.hpp>



int main()
{
	uint8_t frame_buffer[142];

	ccsds::uslp::mchannel_muxer_pchannel_source phys("loraloralora");
	phys.frame_size(sizeof(frame_buffer));
	phys.error_control_len(ccsds::uslp::error_control_len_t::FOUR_OCTETS);

	ccsds::uslp::vchannel_muxer_mchannel_source master(ccsds::uslp::mcid_t(0x100));
	phys.add_mchannel_source(&master);

	ccsds::uslp::map_muxer_vchannel_source virt(ccsds::uslp::gvcid_t(0x100, 0));
	master.add_vchannel_source(&virt);
	virt.frame_seq_no_len(2);

	ccsds::uslp::map_access_source map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0));
	virt.add_map_source(&map_s1);

	ccsds::uslp::map_access_source map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1));
	virt.add_map_source(&map_s2);


	const char data[1024] = "THISISTHEDATA";
	map_s1.add_sdu(
			reinterpret_cast<const uint8_t*>(data),
			sizeof(data),
			ccsds::uslp::qos_t::SEQUENCE_CONTROLLED
	);

	ccsds::uslp::pchannel_frame_params_t frame_params;

	int i = 0;
	while (phys.peek_frame(frame_params))
	{
		std::cout << "peek" << i << "!"
				<< frame_params.channel_id.sc_id() << ","
				<< static_cast<int>(frame_params.channel_id.vchannel_id()) << ","
				<< static_cast<int>(frame_params.channel_id.map_id()) << "; "

				<< static_cast<int>(frame_params.frame_class) << "; "

				<< frame_params.frame_seq_no << "@"
					<< static_cast<int>(frame_params.frame_seq_no_length) << "; "
				<< frame_params.id_is_destination << "; "
				<< frame_params.ocf_present << std::endl;

		phys.pop_frame(frame_buffer);
		i++;
	}


	return 0;
}
