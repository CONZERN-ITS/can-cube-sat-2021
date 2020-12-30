#include <ccsds/sdl/tc/primitives.hpp>

#include <cassert>
#include <cstring>

#include <ccsds/endian.hpp>


namespace ccsds { namespace sdl { namespace tc {


	void frame_header::pack(uint8_t * buffer)
	{
		int bypass_flag = static_cast<int>(_frame_type) & 0x01 >> 0;
		int command_flag = static_cast<int>(_frame_type) & 0x02 >> 1;

		uint16_t buffer01;
		buffer01 = _gvcid.spacecraft_id() & 0x03FF;
		buffer01 |= command_flag << 12;
		buffer01 |= bypass_flag << 13;
		buffer01 |= (_gvcid.packet_version_number() & 0x03) << 14;
		buffer01 = CCSDS_HTONS(buffer01);
		std::memcpy(buffer, &buffer01, sizeof(buffer01));

		uint16_t buffer23;
		buffer23 = _frame_len & 0x03FF;
		buffer23 |= (_gvcid.vchannel_id() & 0x3F) << 10;
		buffer23 = CCSDS_HTONS(buffer23);
		std::memcpy(buffer + sizeof(buffer01), &buffer23, sizeof(buffer23));

		buffer[4] = _seq_no;
	}


	void frame_header::unpack(uint8_t * buffer)
	{
		uint16_t buffer01;
		std::memcpy(&buffer01, buffer, sizeof(buffer01));
		buffer01 = CCSDS_NTOHS(buffer01);
		_gvcid.spacecraft_id(buffer01 & 0x03FF);
		int command_flag = (buffer01 >> 12) & 0x01;
		int bypass_flag = (buffer01 >> 13) & 0x01;
		_gvcid.packet_version_number((buffer01 >> 14) & 0x03);

		int raw_frame_type = (command_flag << 1) | (bypass_flag << 0);
		_frame_type = static_cast<frame_type_t>(raw_frame_type);

		uint16_t buffer23;
		memcpy(&buffer23, buffer + sizeof(buffer01), sizeof(buffer23));
		buffer23 = CCSDS_NTOHS(buffer23);
		_frame_len = buffer23 & 0x03FF;
		_gvcid.vchannel_id((buffer23 >> 10) & 0x03F);

		_seq_no = buffer[4];


	}


}}}

