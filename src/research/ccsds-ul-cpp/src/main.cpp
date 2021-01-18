#include <iostream>
#include <iomanip>
#include <cstring>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>
#include <ccsds/uslp/map/map_access_source.hpp>
#include <ccsds/uslp/map/map_packet_source.hpp>
#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <ccsds/uslp/virtual/map_rr_muxer.hpp>



int main()
{
	uint8_t frame_buffer[142];

	ccsds::uslp::mchannel_rr_muxer phys("loraloralora");
	phys.frame_size(sizeof(frame_buffer));
	phys.error_control_len(ccsds::uslp::error_control_len_t::FOUR_OCTETS);

	ccsds::uslp::vchannel_rr_muxer master(ccsds::uslp::mcid_t(0x100));

	ccsds::uslp::map_rr_muxer virt(ccsds::uslp::gvcid_t(0x100, 0));
	virt.frame_seq_no_len(2);

	ccsds::uslp::map_access_source map_s1(ccsds::uslp::gmap_id_t(0x100, 0, 0));

	ccsds::uslp::map_access_source map_s2(ccsds::uslp::gmap_id_t(0x100, 0, 1));


	phys.add_mchannel_source(&master);
	master.add_vchannel_source(&virt);
	virt.add_map_source(&map_s1);
	virt.add_map_source(&map_s2);

	phys.finalize();

	const char data_sample[] = "\xDE\xAD\xBE\xEF";
	char data[200];
	for (size_t i = 0; i < sizeof(data); i++)
		data[i] = data_sample[i % sizeof(data_sample)];


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

				<< frame_params.frame_seq_no->value() << "@"
					<< static_cast<int>(frame_params.frame_seq_no->value_size()) << "; "
				<< frame_params.id_is_destination << "; "
				<< frame_params.ocf_present << "; ";

		std::memset(frame_buffer, 0, sizeof(frame_buffer));
		phys.pop_frame(frame_buffer);
		i++;

		std::cout << "0x";
		auto fmt_flags = std::cout.flags();
		std::exception_ptr error = nullptr;
		try
		{
			for (size_t i = 0; i < sizeof(frame_buffer); i++)
			{
				std::cout << std::hex
						<< std::setw(2)
						<< std::setfill('0')
						<< (int)frame_buffer[i]
						<< std::dec
				;
			}
		}
		catch (...)
		{
			error = std::current_exception();
		}
		std::cout.flags(fmt_flags);
		if (error) std::rethrow_exception(error);

		std::cout << std::endl;
	}


	return 0;
}
