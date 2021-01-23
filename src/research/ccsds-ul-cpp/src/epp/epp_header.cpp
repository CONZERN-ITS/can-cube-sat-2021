#include <ccsds/epp/epp_header.hpp>

#include <endian.h>

#include <cstring>
#include <cassert>
#include <sstream>
#include <limits>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace epp {


uint16_t header_t::probe_header_size(const uint8_t first_buffer_byte)
{
	// Проверяем pvn
	auto pvn_in_buffer = (first_buffer_byte >> 5) & 0x07;

	if (pvn_in_buffer != pvn)
		return 0; // не подошло.

	// Если подошло - смотрим длину
	uint8_t len_of_len = (first_buffer_byte >> 0) & 0x03;
	switch (len_of_len)
	{
	case 0x00: return 1;
	case 0x01: return 2;
	case 0x02: return 4;
	case 0x04: return 8;
	default:
		assert(false); // быть такого не может
		return 0;
	};
}


uint16_t header_t::size() const
{
	// Если есть это поле - то заголовок полюбому огромный
	if (ccsds_field)
		return 8;

	// Дальше - смотрим что там с длиной пакета
	// Если влезает только в 4 байта - тоже заголовок только огромный
	if (packet_size > 0xFFFFFF) // больше трех байт
		return 8;


	// Окей, все варианты на восьмерку посмотрели
	// Смотрим четверку


	if (protocol_id == static_cast<int>(protocol_id_t::EXTENDED) || protocol_id_extension)
	{
		// Так-то должно быть верно оба условия, но нас устроит хотябы одно.
		// Не будем умничать - будем писать как сказали
		return 4;
	}

	// Аналогично - если есть пользовательское поле - заголовок тоже в 4
	if (user_defined_field)
		return 4;

	// А так же проверим на длину пакета. если не влезает в 2 байт - быть заголовку 4ехбайтным
	if (packet_size > 0xFF)
		return 4;


	// Четверки не будет - смотрим двойку


	// Она будет только в одном случае. Если длина пакета вообще есть какая-то
	if (packet_size > 0)
		return 2;


	// Ну и крайний случай - единичка
	return 1;
}


void header_t::write(uint8_t * buffer, size_t buffer_size) const
{
	// Будем писать по-разному в зависимости от
	const auto header_size = size();

	if (header_size > buffer_size)
	{
		std::stringstream error;
		error << "unable to write epp header. Provided buffer is too small ("
				<< header_size << " < " << buffer_size;
		throw einval_exception(error.str());
	}

	// С первым байтом пока обождем
	// Прогоним свич по длине заголовка
	uint8_t len_of_len;
	switch (header_size)
	{
	case 1: {
		len_of_len = 0x00;
		// Мы тут уже все сделали закончили
	} break;

	case 2: {
		len_of_len = 0x01;
		// Пишем только длину пакета
		buffer[1] = static_cast<uint8_t>(packet_size);
	} break;

	case 4: {
		len_of_len = 0x02;
		// Пишем расширенные поля
		buffer[1] = _make_second_byte();

		// А потом длину пакета
		uint16_t plen = static_cast<uint16_t>(packet_size);
		plen = htobe16(plen);
		std::memcpy(buffer + 2, &plen, sizeof(plen));
	} break;

	case 8: {
		len_of_len = 0x03;
		// Снова пишем расширенные поля
		buffer[1] = _make_second_byte();

		// Потом поле CCSDS
		uint16_t ccsds_fld = htobe16(ccsds_field ? ccsds_field.value() : 0x0000);
		std::memcpy(buffer + 2, &ccsds_fld, sizeof(ccsds_fld));

		// А вот теперь длину пакета
		uint32_t plen = htobe32(packet_size);
		std::memcpy(buffer + 2 + sizeof(ccsds_fld), &plen, sizeof(plen));
	}
	break;

	default:
		// Это какая-то очень нездоровая петрушка
		assert(false);
	break;

	}; // switch

	// А теперь напишем и первый байт
	uint8_t byte1 = 0;
	byte1 |= (pvn & 0x07) << 5;
	byte1 |= (protocol_id & 0x07) << 2;
	byte1 |= (len_of_len & 0x03) << 0; // тут пишется на единичку меньше чем есть
	buffer[0] = byte1;

}


void header_t::read(const uint8_t * buffer, size_t buffer_size)
{
	if (0 == buffer_size)
	{
		// Ну это совсем наглость
		std::stringstream error;
		error << "unable to read epp header. Provided buffer has zero size! (AND THIS IS UNACCEPTABLE)";
		throw einval_exception(error.str());
	}

	// читаем первый байт...
	const uint8_t byte1 = buffer[0];
	const auto header_size = probe_header_size(byte1);
	if (header_size == 0)
		throw einval_exception("invalid epp packet header. PVN is invalid");

	// Снова проверяем на длину буфера...
	if (header_size > buffer_size)
	{
		std::stringstream error;
		error << "unable to read epp header. Provided buffer is too small ("
				<< header_size << " < " << buffer_size;
		throw einval_exception(error.str());
	}

	// Читаем прочие поля
	// len_of_len мы уже вчитали, когда считали header_size
	protocol_id = (byte1 >> 2) & 0x07;

	user_defined_field.reset();
	protocol_id_extension.reset();
	ccsds_field.reset();
	switch (header_size)
	{
	case 1: { // заголовок в 1 байт
		// Вообще никаких больше данных нет
		packet_size = 0;
	} break;

	case 2: { // заголовок в 2 байта
		// есть байтик длины пакета
		packet_size = buffer[1];
	} break;

	case 4: { // заголовок в 4 байта
		// Есть дополнительное поле
		_load_second_byte(buffer[1]);

		// Есть длина пакета аж в два байта
		uint16_t plen;
		std::memcpy(&plen, buffer + 2, sizeof(plen));
		packet_size = be16toh(plen);
	} break;

	case 8: { // заголовок аж в 8 байт
		// Есть дополнительное поле
		_load_second_byte(buffer[1]);

		// Есть поле ccsds
		uint16_t ccsds_fld;
		std::memcpy(&ccsds_fld, buffer + 2, sizeof(ccsds_fld));
		ccsds_field = be16toh(ccsds_fld);

		// Есть супер большая длина пакета
		uint32_t plen;
		std::memcpy(&plen, buffer + 2 + sizeof(ccsds_fld), sizeof(plen));
		packet_size = be16toh(plen);
	} break;

	default:
		assert(false);
		break; // Быть того не может
	}; // switch
}


void header_t::real_packet_size(uint64_t value)
{
	if (0 == value)
		throw einval_exception("epp real packet size can`t be zero");

	constexpr uint64_t max_size = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1;
	if (value > max_size)
	{
		std::stringstream error;
		error << "epp real packet size is too big (" << value << "). Allowed maximum is "
				<< max_size;
		throw einval_exception(error.str());
	}

	packet_size = static_cast<decltype(packet_size)>(value - 1);
}


uint64_t header_t::accomadate_to_payload_size(uint64_t payload_size)
{
	// Действуем итеративно
	packet_size = payload_size;
	auto header_size = size();

	// FIXME: У меня сильные подозрения что тут можно и не вайлом
	// а просто в два ифа, но проверять это некогда
	while(true)
	{
		packet_size = payload_size + header_size - 1; // -1 по спеке
		const auto header_size2 = size();
		if (header_size2 == header_size)
			break;

		// Если после назначения длины пакета длина заголовка увеличилась
		// Нужно пересчитать длину пакета и снова проверить..
		header_size = header_size2;
		continue;
	}

	return real_packet_size();
}


uint8_t header_t::_make_second_byte() const
{
	// Если расширенный айдишник не указан, а нам надо что-то писать - нужно писать нули
	uint8_t pid_ext = protocol_id_extension ? protocol_id_extension.value() : 0x00;
	// Если пользвательское поле не указано, а нам опять надо что-то писать - тоже пишем нули
	uint8_t usr_fld = user_defined_field ? user_defined_field.value() : 0x00;

	uint8_t byte2 = 0;
	byte2 |= (pid_ext & 0x0F) << 0;
	byte2 |= (usr_fld & 0x0F) << 4;

	return byte2;
}


void header_t::_load_second_byte(uint8_t byte2)
{
	protocol_id_extension = (byte2 >> 0) & 0x0F;
	user_defined_field = (byte2 >> 4) & 0x0F;
}


}}
