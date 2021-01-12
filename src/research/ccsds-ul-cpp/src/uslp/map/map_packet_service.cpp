#include <ccsds/uslp/map/map_packet_service.hpp>

#include <cassert>
#include <cstring>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/map/detail/tfdf_header.hpp>
#include <ccsds/uslp/map/detail/epp_header.hpp>


namespace ccsds { namespace uslp {

map_packet_service::map_packet_service(gmap_id_t map_id_)
	: map_service(map_id_), _data_queue()
{
	// С порога ставим себе размер зоны, чтобы в нее хоть заголовок влез
	tfdf_size(detail::tfdf_header::full_size);
}


void map_packet_service::tfdf_size(uint16_t value)
{
	// Проверка на то, что мы можем столько
	if (value < detail::tfdf_header::full_size)
		throw einval_exception("map_packet service requires no less than 4 bytes for tfdf zone size");

	this->map_service::tfdf_size(value);
}


bool map_packet_service::peek_tfdf()
{
	// Тут оптимизации не будет :c
	tfdf_params dummy;
	return peek_tfdf(dummy);
}


bool map_packet_service::peek_tfdf(tfdf_params & params)
{
	// У нас вообще что-нибудь есть там?
	if (_data_queue.empty())
		return false;

	// Окей, хоть один пакет у нас есть, значит собственно с qos следующего фрейма все понятно
	params.qos = _data_queue.front().qos;

	// Смотрим какой объем нам нужно занять
	const size_t tfdz_size = _tfdz_size();

	// Теперь смотрим то что мы должны отправить безразрывно - оно больше чем нужно набрать?
	// То есть - нужно ли нам блокировать на себя канал?

	// Для этого проходим по всем пакетам, что есть в буфере
	// И смотрим набирается ли нужное количество байт
	size_t continous_stream_size = 0;
	qos_t prev_qos = _data_queue.front().qos;
	for (auto itt = std::cbegin(_data_queue); itt != std::cend(_data_queue); itt++)
	{
		// Флаш и "отпускание канала" произойдет либо когда у нас кончатся данные. либо когда
		// У следующих пакетов сменится qos
		if (itt->qos != prev_qos)
			break;

		// Если мы видим что уже набрали данных больше чем на один фрейм -
		// продолжать нет смысла, останавливаем цикл
		if (continous_stream_size >= tfdz_size)
			break;

		// Окей, считаем этот пакет на отправку и продолжаем
		continous_stream_size += itt->packet.size();
	}

	// Мы посчитали сколько байт мы будем отправлять неразрывно
	// Теперь легко сказать заблокируем ли мы канал
	params.channel_lock = continous_stream_size > tfdz_size;
	return true;
}


void map_packet_service::pop_tfdf(uint8_t * tfdf_buffer)
{
	// Если ничего нет - ничего и не дадим. Хотя по чесноку - этот метод не должны вызывать
	// если ничего нет
	if (_data_queue.empty())
		return;

	// Отступаем место под заголовок
	// У нас всегда полный размер заголовка
	uint8_t * output_buffer = tfdf_buffer + detail::tfdf_header::full_size;
	uint16_t output_buffer_size = _tfdz_size();
	// Это тоже запомним, пригодится
	const uint8_t * const tfdz_start = output_buffer;

	// Идем по пакетам и копируем их
	uint16_t first_valid_header_offset = 0xFFFF; // Специальное значение, показывающее NULL
	while (!_data_queue.empty())
	{
		auto & data_unit = _data_queue.front();

		// Определяем сколько мы можем скопировать
		const uint16_t to_copy_size = static_cast<uint16_t>(
			std::min(
					static_cast<size_t>(output_buffer_size),
					data_unit.packet.size()
			)
		);

		// Если мы сейчас пишем первый правильный заголовок - нужно бы это указать
		if (first_valid_header_offset == 0xFFFF && data_unit.packet.size() == data_unit.original_packet_size)
			first_valid_header_offset = static_cast<uint16_t>(output_buffer - tfdz_start);

		// Копируем
		const auto to_copy_begin = std::cbegin(data_unit.packet);
		const auto to_copy_end = std::next(to_copy_begin, to_copy_size);
		std::copy(to_copy_begin, to_copy_end, output_buffer);

		// Дропаем из деки скопированное и пересчитываем параметры выходного буфера
		data_unit.packet.erase(to_copy_begin, to_copy_end);
		output_buffer += to_copy_size;
		output_buffer_size -= to_copy_size;

		// если мы скопировали весь пакет - дропаем его
		if (data_unit.packet.empty())
			_data_queue.pop_front();

		// Если в выходном буфере не осталось места - останавливаемся
		if (0 == output_buffer_size)
			break;
	}

	// Если мы записали все что могли, а в выходном буфере все равно осталось место
	// Добиваем его idle пакетом
	if (output_buffer_size)
		_write_idle_packet(output_buffer, output_buffer_size);

	// Опять проверяем на первый валидный заголовок
	if (first_valid_header_offset == 0xFFFF)
		first_valid_header_offset = static_cast<uint16_t>(output_buffer - tfdz_start);

	// Наконец-то пишем заголовок
	detail::tfdf_header header;
	header.ctr_rule = detail::tfdz_construction_rule_t::PACKETS_SPANNING_MULTIPLE_FRAMES;
	header.upid = detail::upid_t::CCSDS_PACKETS;
	header.first_header_offset = first_valid_header_offset;
	header.write(tfdf_buffer);
	// Готово!
}


uint16_t map_packet_service::_tfdz_size() const
{
	return map_service::tfdf_size() - detail::tfdf_header::full_size; // У нас всегда полный заголовок
}


void map_packet_service::_write_idle_packet(uint8_t * buffer, uint16_t idle_packet_size) const
{
	if (0 == idle_packet_size)
		return;

	detail::epp_header header;
	header.protocol_id = detail::epp_protocol_id_t::IDLE;
	if (1 == idle_packet_size)
		header.packet_len = 0; // Особый случай, будем писать только заголовок
	else
		header.packet_len = idle_packet_size; // Во всех остальных случаях пишем честно

	// Теперь пишем заголовок
	header.write(buffer);

	// Тело пакета заполним меандром, чтобы было красиво (и радио канал лучше ловил битсинк)
	uint8_t * const pbody_buffer = buffer + header.size();
	const uint16_t pbody_size = idle_packet_size - header.size();
	std::memset(pbody_buffer, pbody_size, 0xAA);
}

}}
