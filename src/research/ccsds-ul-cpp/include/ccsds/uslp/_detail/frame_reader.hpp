#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_

#include <cstdint>
#include <optional>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>


namespace ccsds { namespace uslp { namespace detail {


class frame_reader
{
public:
	frame_reader(
			uint8_t * frame_buffer, size_t frame_buffer_size,
			uint16_t insert_zone_size
	);

	const uint8_t * frame_buffer() { return _frame_buffer; }
	size_t frame_buffer_size() { return _frame_buffer_size; }

	const uint8_t * raw_frame_headers() { return _frame_buffer; }
	uint16_t raw_frame_headers_size();

	uint8_t frame_version_no();
	gmap_id_t service_id();
	bool sid_is_destination();
	frame_class_t frame_class();
	std::optional<int64_t> frame_seq_no();

	const uint8_t * insert_zone();
	uint16_t insert_zone_size();

	const uint8_t * data_field();
	uint16_t data_field_size();

	const uint8_t * ocf_field();
	uint16_t ocf_field_size();

private:
	struct basic_header
	{
		uint8_t frame_version_no;
		gmap_id_t gmap_id;
		bool id_is_destination;
	};

	struct extended_header
	{
		frame_class_t frame_class;
		uint16_t frame_len;
		bool ocf_present;
		std::optional<uint64_t> frame_seq_no;
	};

	bool _parse_basic_header(const uint8_t * frame_buffer, basic_header & header);
	uint16_t _parse_extended_header(const uint8_t * frame_buffer, extended_header & header);

	const uint8_t * _frame_buffer;
	const size_t _frame_buffer_size;
	const uint16_t _insert_zone_size;

	basic_header _basic_header;
	std::optional<extended_header> _extended_header;
	uint16_t _headers_size;

};


}}}




#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_ */
