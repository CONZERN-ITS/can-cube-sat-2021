#include <ccsds/uslp/map/map_packet_sink.hpp>

#include <sstream>
#include <cassert>
#include <array>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>


namespace ccsds { namespace uslp {


map_packet_sink::map_packet_sink(gmap_id_t channel_id) // @suppress("Class members should be properly initialized")
	: map_sink(channel_id)
{

}


void map_packet_sink::max_packet_size(size_t value)
{
	if (is_finalized())
	{
		std::stringstream error;
		error << "Can`t use max_packet_size on map_packet_sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_max_packet_size = value;
}


void map_packet_sink::emit_idle_packets(bool value)
{
	if (is_finalized())
	{
		std::stringstream error;
		error << "Can`t use emit_idle_packets on map_packet_sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	map_packet_sink::_emit_idle_packets = value;
}


void map_packet_sink::finalize_impl()
{
	map_sink::finalize_impl();
}


void map_packet_sink::push_impl(
		const input_map_frame_params & params,
		const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	if (detail::tfdf_header_t::full_size >= tfdf_buffer_size)
	{
		// Этот фрейм не может быть валидным, так как в него ничего не влезает
		_flush_accum(map_sink_event_data_unit::release_reason_t::SDU_TERMINATED);
		return;
	}

	// Читаем заголовок tfdf
	detail::tfdf_header_t tfdf_header;
	tfdf_header.read(tfdf_buffer, true);
	const uint8_t * const tfdz_start = tfdf_buffer + tfdf_header.size();
	const size_t tfdz_size = tfdf_buffer_size - tfdf_header.size();

	// Если мы пустые
	if (_accumulator.empty())
	{
		// Смотрим есть ли в этом фрейме начало пакета
		assert(tfdf_header.first_header_offset.has_value());
		if (tfdf_header.first_header_offset.value() > tfdz_size)
		{
			// Нету. Ну... искать не будем
			return;
		}

		// А вот если есть - тут по-подробнее
		const uint8_t * const packet_start = tfdz_start + *tfdf_header.first_header_offset;
		const uint8_t * const packet_part_end = tfdz_start + tfdz_size;
		_consume_bytes(packet_start, packet_part_end);
	}


	if (_prev_frame_seq_no.has_value() != params.frame_seq_no.has_value())
	{
		// Ой, так быть не должно
		_flush_accum(map_sink_event_data_unit::release_reason_t::SDU_TERMINATED);
	}

}


void map_packet_sink::_consume_bytes(const uint8_t * begin, const uint8_t * end)
{
	// Ну, на последовательность все проверяли выше
	// Поэтому мы просто жрем все байты в наш буфер
	std::copy (begin, end, std::back_inserter(_accumulator));

	// А теперь пробуем разгрести что нашлось
	_parse_packets();
}


void map_packet_sink::_parse_packets()
{

again:
	if (!_current_packet_header.has_value())
	{
		// Если у нас еще нет заголовка
		// Смотрим хватает ли у нас на него байт
		// Мы работаем только с epp пакетами
		const auto header_size = detail::epp_header_t::probe_header_size(_accumulator.front());
		if (0 == header_size)
		{
			// Значит это не заголовок... Сбрасывем все
			_flush_accum(map_sink_event_data_unit::release_reason_t::BAD_PACKET_HEADER);
			return;
		}

		// У нас уже достаточно байт?
		if (_accumulator.size() < header_size)
		{
			// Тут мало что можно сделать.. Будем подождать
			return;
		}

		// Поскольку дека не константна по длине... сделаем небольшую копию байтиков
		std::array<uint8_t, detail::epp_header_t::maximum_size> epp_header_buffer = {0};
		std::copy(
				_accumulator.begin(),
				std::next(_accumulator.begin(), header_size),
				epp_header_buffer.begin()
		);

		// Отлично, пробуем разобрать
		detail::epp_header_t epp_header;
		try
		{
			epp_header.read(epp_header_buffer.data(), header_size);
		}
		catch (std::exception & e)
		{
			// К сожалению - весь наш буфер уходит в труху, потому что мы не можем найти
			// границы никакого пакета
			_flush_accum(map_sink_event_data_unit::release_reason_t::BAD_PACKET_HEADER);
			return;
		}

		// Теперь у нас есть заголовок!
		_current_packet_header = epp_header;
	}

	// Раз у нас есть заголовок - то может у нас хватает байтиков и на сам пакет?
	const auto current_packet_size = _current_packet_header->real_packet_size();
	if (current_packet_size < _accumulator.size())
	{
		auto current_packet_end = std::next(_accumulator.cbegin(), current_packet_size);
		// Чудесно! Только секундочку... А это не idle пакет?
		// Такие мы пользователю давать не будем
		if (!_emit_idle_packets && detail::epp_protocol_id_t::IDLE == _current_packet_header->protocol_id)
		{
			_drop_accum(current_packet_end);
		}
		else
		{
			_flush_accum(current_packet_end, map_sink_event_data_unit::release_reason_t::SDU_COMPLETE);
		}
	}

	// Выгребаем дальше
	goto again;
}


void map_packet_sink::_flush_accum(map_sink_event_data_unit::release_reason_t reason)
{
	_flush_accum(_accumulator.end(), reason);
}


void map_packet_sink::_flush_accum(
		accum_t::const_iterator flush_zone_end, map_sink_event_data_unit::release_reason_t reason
)
{
	if (_accumulator.empty())
		return; // Нечего флашить

	map_sink_event_data_unit event;
	event.qos = _prev_frame_qos;
	event.release_reason = reason;

	event.data.reserve(std::distance(_accumulator.cbegin(), flush_zone_end));
	std::copy(_accumulator.cbegin(), flush_zone_end, std::back_inserter(event.data));
	_drop_accum(flush_zone_end);

	emit_event(event);
}


void map_packet_sink::_drop_accum(accum_t::const_iterator drop_zone_end)
{
	_accumulator.erase(_accumulator.begin(), drop_zone_end);
	_current_packet_header.reset();
}



}}
