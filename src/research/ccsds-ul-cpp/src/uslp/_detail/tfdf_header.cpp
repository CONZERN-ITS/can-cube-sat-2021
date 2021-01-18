#include <ccsds/uslp/_detail/tfdf_header.hpp>

#include <endian.h>

#include <cstring>


namespace ccsds { namespace uslp { namespace detail {


void tfdf_header_t::write(uint8_t * buffer) const
{
	uint8_t byte = 0;
	byte |= (this->upid & 0x1F) << 0;
	byte |= (this->ctr_rule & 0x07) << 5;
	buffer[0] = byte;

	if (first_header_offset)
	{
		uint16_t word;
		word = htobe16(this->first_header_offset.value());
		std::memcpy(buffer + sizeof(byte), &word, sizeof(word));
	}
}


void tfdf_header_t::read(const uint8_t * buffer, bool has_header_offset)
{
	uint8_t byte;
	byte = buffer[0];
	upid = (byte >> 0) & 0x1F;
	ctr_rule = (byte >> 5) & 0x07;

	if (has_header_offset)
	{
		uint16_t word;
		std::memcpy(&word, buffer + sizeof(byte), sizeof(word));
		first_header_offset = be16toh(word);
	}
	else
	{
		first_header_offset.reset();
	}
}


}}}
