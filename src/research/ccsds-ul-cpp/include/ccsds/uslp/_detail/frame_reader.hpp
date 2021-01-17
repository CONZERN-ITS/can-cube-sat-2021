#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_

#include <cstdint>
#include <optional>

#include <ccsds/uslp/_detail/tf_header.hpp>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>


namespace ccsds { namespace uslp { namespace detail {


class frame_reader
{
public:
	frame_reader(
			const uint8_t * frame_buffer, size_t frame_buffer_size,
			uint16_t insert_zone_size, error_control_len_t error_control_len
	);

	const uint8_t * frame_buffer() const { return _frame_buffer; }
	size_t frame_buffer_size() const { return _frame_buffer_size; }

	const uint8_t * raw_frame_headers() const { return _frame_buffer; }
	uint16_t raw_frame_headers_size() const;
	const tf_header_t & frame_headers() const { return _frame_header; }

	uint8_t frame_version_no() const;
	gmap_id_t service_id()  const;
	bool sid_is_destination()  const;
	std::optional<frame_class_t> frame_class()  const;
	std::optional<int64_t> frame_seq_no() const;

	const uint8_t * insert_zone() const;
	uint16_t insert_zone_size() const;

	const uint8_t * data_field() const;
	uint16_t data_field_size() const;

	const uint8_t * error_control_field() const;
	error_control_len_t error_control_field_size() const;

	const uint8_t * ocf_field() const;
	static constexpr uint16_t ocf_field_size = sizeof(uint32_t);

protected:
	uint16_t _ocf_field_size() const;

private:
	tf_header_t _frame_header;

	const uint8_t * _frame_buffer;
	const size_t _frame_buffer_size;
	const uint16_t _insert_zone_size;
	error_control_len_t _error_control_len;
};


}}}




#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_FRAME_READER_HPP_ */
