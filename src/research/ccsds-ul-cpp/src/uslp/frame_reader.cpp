#include <ccsds/uslp/frame_reader.hpp>
#include <endian.h>
#include <cstring>


namespace ccsds { namespace uslp {

frame_reader::frame_reader(
		uint8_t * frame_buffer, size_t frame_buffer_size, uint16_t insert_zone_size
)
		: _frame_buffer(frame_buffer),
		  _frame_buffer_size(frame_buffer_size),
		  _insert_zone_size(insert_zone_size)
{
	bool have_extended_header = _parse_basic_header(_frame_buffer, _basic_header);

	uint16_t ext_headers_size = 0;
	if (have_extended_header)
	{
		_extended_header.emplace();
		ext_headers_size = _parse_extended_header(_frame_buffer, *_extended_header);
	}

	_headers_size = sizeof(uint32_t) + ext_headers_size;
}


uint16_t frame_reader::raw_frame_headers_size()
{
	return _headers_size;
}


uint8_t frame_reader::frame_version_no()
{
	return _basic_header.frame_version_no;
}


gmap_id_t frame_reader::service_id()
{
	return _basic_header.gmap_id;
}


bool frame_reader::sid_is_destination()
{
	return _basic_header.id_is_destination;
}


frame_class_t frame_reader::frame_class()
{
	return _extended_header ? _extended_header->frame_class : frame_class_t::EXPEDITED_PAYLOAD;
}


std::optional<int64_t> frame_reader::frame_seq_no()
{
	return _extended_header ? _extended_header->frame_seq_no : std::nullopt;
}


const uint8_t * frame_reader::insert_zone()
{
	auto size = insert_zone_size();
	if (!size)
		return nullptr;
	else
		return _frame_buffer + _headers_size;
}


uint16_t frame_reader::insert_zone_size()
{
	if (!_extended_header)
	{
		// фреймы с коротким заголовком не несут инсерт зоны
		return 0;
	}

	return _insert_zone_size;
}


const uint8_t * frame_reader::data_field()
{
	return _frame_buffer + raw_frame_headers_size() + insert_zone_size();
}


uint16_t frame_reader::data_field_size()
{
	// так то все что осталось - то и data field
	return _frame_buffer_size - raw_frame_headers_size() - insert_zone_size() - ocf_field_size();
}


const uint8_t * frame_reader::ocf_field()
{
	if (!_extended_header)
	{
		// Фреймы с коротким заголовоком не несут OCF полей
		return nullptr;
	}


	if (!_extended_header->ocf_present)
	{
		// Если явно не указано, что это поле есть - значит его нет
		return nullptr;
	}

	return nullptr;
}


uint16_t frame_reader::ocf_field_size()
{
	return 0;
}


bool frame_reader::_parse_basic_header(const uint8_t * frame_buffer, basic_header & header)
{
	uint32_t word;
	std::memcpy(&word, _frame_buffer, sizeof(word));
	word = be32toh(word);

	bool have_extension = (word >> 0) & 0x0001 ? false : true; // внимание - тут инверсия
	header.gmap_id.map_id((word >> 1) & 0x000F);
	header.gmap_id.vchannel_id((word >> 5) & 0x003F);
	header.id_is_destination = (word >> 11) & 0x0001 ? true : false;
	header.gmap_id.sc_id((word >> 12) & 0x00FF);
	header.frame_version_no = (word >> 28) & 0x000F;

	return have_extension;
}


uint16_t frame_reader::_parse_extended_header(const uint8_t * frame_buffer, extended_header & header)
{
	// Считаем начало вторичного заголовка - сразу после первичного
	const uint8_t * ext_header_start = _frame_buffer + sizeof(uint32_t);

	// Первое поле - длина кадра
	uint16_t frame_len;
	std::memcpy(&frame_len, ext_header_start, sizeof(uint16_t));
	frame_len = be32toh(frame_len);
	header.frame_len = frame_len + 1; // Потому что именно так оно и считается

	// Второе поле - различные флаги
	uint8_t frame_flags = *(ext_header_start + sizeof(uint16_t));

	// Считаем класс фрейма
	bool bypass_flag = (frame_flags >> 7) & 0x01 ? true : false;
	bool command_flag = (frame_flags >> 6) & 0x01 ? true : false;
	if (bypass_flag && command_flag)
		header.frame_class = frame_class_t::EXPEDITED_COMMAND;
	else if (bypass_flag && !command_flag)
		header.frame_class = frame_class_t::EXPEDITED_PAYLOAD;
	else if (!bypass_flag && !command_flag)
		header.frame_class = frame_class_t::CONTROLLED_PAYLOAD;
	else // if (!bypass_flag && command_flag)
		header.frame_class = frame_class_t::RESERVED;

	// Дальше два бита резерва

	// Теперь смотрим есть ли в кадре OCF поле
	header.ocf_present = (frame_flags >> 3) & 0x01 ? true : false;

	// смотрим длину поля с номером кадра
	int frame_no_len = (frame_flags >> 0) & 0x7;
	if (frame_no_len)
	{
		const uint8_t * const frame_no_bytes_start =
				ext_header_start + sizeof(uint16_t) + sizeof(uint8_t)
		;
		uint64_t frame_no = 0;
		for (int i = 0; i < frame_no_len; i++)
		{
			// Считываем очередной байт
			const uint8_t byte = frame_no_bytes_start[i];
			// Впихиваем его в итоговое число в соответствующий разряд
			frame_no = frame_no | static_cast<uint64_t>(byte) << (frame_no_len - 1 - i);
		}

		header.frame_seq_no = frame_no;
	}

	return sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + frame_no_len;
}



}}
