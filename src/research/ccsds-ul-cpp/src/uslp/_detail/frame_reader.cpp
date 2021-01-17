#include <ccsds/uslp/_detail/frame_reader.hpp>
#include <endian.h>
#include <cstring>


namespace ccsds { namespace uslp { namespace detail {


frame_reader::frame_reader(
		const uint8_t * frame_buffer, size_t frame_buffer_size,
		uint16_t insert_zone_size, error_control_len_t error_control_len
)
		: _frame_buffer(frame_buffer),
		  _frame_buffer_size(frame_buffer_size),
		  _insert_zone_size(insert_zone_size),
		  _error_control_len(error_control_len)
{
	_frame_header.read(frame_buffer);
}


uint16_t frame_reader::raw_frame_headers_size() const
{
	return _frame_header.size();
}


uint8_t frame_reader::frame_version_no() const
{
	return _frame_header.frame_version_no;
}


gmap_id_t frame_reader::service_id() const
{
	return _frame_header.gmap_id;
}


bool frame_reader::sid_is_destination() const
{
	return _frame_header.id_is_destination;
}


std::optional<frame_class_t> frame_reader::frame_class() const
{
	if (_frame_header.ext)
		return _frame_header.ext->frame_class;
	else
		return std::nullopt;
}


std::optional<int64_t> frame_reader::frame_seq_no() const
{
	return _frame_header.ext ? _frame_header.ext->frame_seq_no() : std::nullopt;
}


const uint8_t * frame_reader::insert_zone() const
{
	auto size = insert_zone_size();
	if (!size)
		return nullptr;
	else
		return _frame_buffer + _frame_header.size();
}


uint16_t frame_reader::insert_zone_size() const
{
	if (!_frame_header.ext)
	{
		// фреймы с коротким заголовком не несут инсерт зоны
		return 0;
	}

	return _insert_zone_size;
}


const uint8_t * frame_reader::data_field() const
{
	return _frame_buffer + raw_frame_headers_size() + insert_zone_size();
}


uint16_t frame_reader::data_field_size() const
{
	// так то все что осталось - то и data field
	return _frame_buffer_size
			- raw_frame_headers_size() - insert_zone_size() - _ocf_field_size() -
			static_cast<uint16_t>(error_control_field_size())
	;
}


const uint8_t * frame_reader::error_control_field() const
{
	if (!static_cast<uint16_t>(error_control_field_size()))
		return nullptr;

	return _frame_buffer
			+ raw_frame_headers_size() + insert_zone_size() + data_field_size() + _ocf_field_size()
	;
}


error_control_len_t frame_reader::error_control_field_size() const
{
	if (!_frame_header.ext)
	{
		// Такие фреймы не несут контрольной суммы
		return error_control_len_t::ZERO;
	}

	return _error_control_len;
}


const uint8_t * frame_reader::ocf_field() const
{
	if (!_ocf_field_size())
		return nullptr;

	return _frame_buffer + raw_frame_headers_size() + insert_zone_size() + data_field_size();
}


uint16_t frame_reader::_ocf_field_size() const
{
	return _frame_header.ext && _frame_header.ext->ocf_present ? sizeof(uint32_t) : 0;
}

}}}
