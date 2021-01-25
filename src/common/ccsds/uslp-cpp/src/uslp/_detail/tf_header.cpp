#include <ccsds/uslp/_detail/tf_header.hpp>

#include <endian.h>

#include <sstream>
#include <cstring>
#include <cassert>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp { namespace detail {


uint16_t tf_header_extended_part_t::size() const noexcept
{
	return static_size + (frame_seq_no ? frame_seq_no->value_size() : 0);
}


void tf_header_extended_part_t::write(uint8_t * buffer) const noexcept
{
	uint8_t * const ext_header_start = buffer;

	// Первое поле - длина кадра
	const uint16_t frame_len = htobe16(this->frame_len);
	std::memcpy(buffer, &frame_len, sizeof(frame_len));

	// Второе поле - различные флаги
	uint8_t frame_flags = 0;

	bool bypass_flag;
	bool command_flag;
	switch (frame_class)
	{
	case frame_class_t::EXPEDITED_COMMAND:
		bypass_flag = true;
		command_flag = true;
		break;

	case frame_class_t::EXPEDITED_PAYLOAD:
		bypass_flag = true;
		command_flag = false;
		break;

	case frame_class_t::CONTROLLED_PAYLOAD:
		bypass_flag = false;
		command_flag = false;
		break;

	case frame_class_t::RESERVED:
		bypass_flag = false;
		command_flag = true;
		break;

	default:
		assert(false);
	}


	uint8_t frame_seq_no_len = 0;
	uint64_t frame_seq_no_value;
	if (frame_seq_no)
	{
		frame_seq_no_len = frame_seq_no->value_size();
		frame_seq_no_value = frame_seq_no->value(); //  не .value, а ->value. Это важно
	}

	frame_flags |= (bypass_flag ? 0x01 : 0x00) << 7;
	frame_flags |= (command_flag ? 0x01 : 0x00) << 6;
	frame_flags |= (ocf_present ? 0x01 : 0x00) << 3;
	frame_flags |= (frame_seq_no_len & 0x7) << 0;
	std::memcpy(buffer + sizeof(uint16_t), &frame_flags, sizeof(frame_flags));

	// теперь самое неприятное - пишем собственно длину кадра
	uint8_t * const frame_no_bytes_start =
			ext_header_start + sizeof(uint16_t) + sizeof(uint8_t);

	for (uint8_t i = 0; i < frame_seq_no_len; i++)
	{
		// Раскладываем со старшего байта
		uint8_t byte = (frame_seq_no_value >> (frame_seq_no_len - 1 - i)*8) & 0xFF;
		frame_no_bytes_start[i] = byte;
	}

	// На этом все
}


void tf_header_extended_part_t::read(const uint8_t * buffer)
{
	const uint8_t * const ext_header_start = buffer;

	// Будем грузить все в кандидата. Потому что хотим зачем-то exception safety
	tf_header_extended_part_t candidate;

	// Первое поле - длина кадра
	uint16_t frame_len;
	std::memcpy(&frame_len, ext_header_start, sizeof(uint16_t));
	frame_len = be16toh(frame_len);
	candidate.frame_len = frame_len;

	// Второе поле - различные флаги
	uint8_t frame_flags = *(ext_header_start + sizeof(uint16_t));

	// Считаем класс фрейма
	bool bypass_flag = (frame_flags >> 7) & 0x01 ? true : false;
	bool command_flag = (frame_flags >> 6) & 0x01 ? true : false;
	if (bypass_flag && command_flag)
		candidate.frame_class = frame_class_t::EXPEDITED_COMMAND;
	else if (bypass_flag && !command_flag)
		candidate.frame_class = frame_class_t::EXPEDITED_PAYLOAD;
	else if (!bypass_flag && !command_flag)
		candidate.frame_class = frame_class_t::CONTROLLED_PAYLOAD;
	else // if (!bypass_flag && command_flag)
		candidate.frame_class = frame_class_t::RESERVED;

	// Дальше два бита резерва

	// Теперь смотрим есть ли в кадре OCF поле
	candidate.ocf_present = (frame_flags >> 3) & 0x01 ? true : false;

	// смотрим длину поля с номером кадра
	std::optional<uint16_t> frame_seq_no_value;
	const uint16_t frame_seq_no_len = (frame_flags >> 0) & 0x7;
	if (frame_seq_no_len)
	{
		const uint8_t * const frame_no_bytes_start =
				ext_header_start + sizeof(uint16_t) + sizeof(uint8_t)
		;
		uint64_t frame_no = 0;
		for (int i = 0; i < frame_seq_no_len; i++)
		{
			// Считываем очередной байт
			const uint8_t byte = frame_no_bytes_start[i];
			// Впихиваем его в итоговое число в соответствующий разряд
			frame_no = frame_no | static_cast<uint64_t>(byte) << (frame_seq_no_len - 1 - i)*8;
		}

		frame_seq_no_value = frame_no;
	}

	if (frame_seq_no_value)
		candidate.frame_seq_no.emplace(*frame_seq_no_value, frame_seq_no_len);

	std::swap(*this, candidate); // @suppress("Invalid arguments")
}


/*static*/ uint16_t tf_header_t::extended_size_forecast(uint8_t frame_seq_no_len)
{
	if (frame_seq_no_len > 7)
	{
		std::stringstream error;
		error << "invalid frame_seq_no_len value: " << static_cast<int>(frame_seq_no_len) << ". "
				<< "frame_seq_no_len should be in range [0, 7]";
		throw einval_exception(error.str());
	}

	return static_size + tf_header_extended_part_t::static_size + frame_seq_no_len;
}


uint16_t tf_header_t::size() const
{
	uint16_t retval = static_size;
	if (ext)
		retval += ext->size();

	return retval;
}


void tf_header_t::write(uint8_t * buffer) const
{
	// Пишем первичный заголовок
	uint32_t word = 0;

	const uint8_t have_extension = ext.has_value() ? 0x00 : 0x01; // Внимание - тут инверсия
	word |= (have_extension & 0x0001) << 0;
	word |= (gmap_id.map_id() & 0x000F) << 1;
	word |= (gmap_id.vchannel_id() & 0x003F) << 5;
	word |= (id_is_destination ? 0x0001 : 0x00) << 11;
	word |= (gmap_id.sc_id() & 0xFFFF) << 12;
	word |= (frame_version_no & 0x000F) << 28;

	word = htobe32(word);
	std::memcpy(buffer, &word, sizeof(word));

	// Расширенный
	if (ext)
		ext->write(buffer + tf_header_t::static_size);
}


void tf_header_t::read(const uint8_t * buffer)
{
	uint32_t word;
	std::memcpy(&word, buffer, sizeof(word));
	word = be32toh(word);

	tf_header_t candidate;

	bool have_extension = (word >> 0) & 0x0001 ? false : true; // внимание - тут инверсия
	candidate.gmap_id.map_id((word >> 1) & 0x000F);
	candidate.gmap_id.vchannel_id((word >> 5) & 0x003F);
	candidate.id_is_destination = (word >> 11) & 0x0001 ? true : false;
	candidate.gmap_id.sc_id((word >> 12) & 0xFFFF);
	candidate.frame_version_no = (word >> 28) & 0x000F;

	if (have_extension)
	{
		candidate.ext.emplace();
		candidate.ext->read(buffer + tf_header_t::static_size);
	}

	std::swap(*this, candidate);  // @suppress("Invalid arguments")
}



}}}
