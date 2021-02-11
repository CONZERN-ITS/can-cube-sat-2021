#include <ccsds/uslp/map/map_access_acceptor.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>


namespace ccsds { namespace uslp {


map_access_acceptor::map_access_acceptor(gmapid_t mapid_)
		: map_acceptor(mapid_),
		  _prev_frame_qos(qos_t::EXPIDITED) // Просто чтобы не иметь UB
{

}


void map_access_acceptor::finalize_impl()
{
	map_acceptor::finalize_impl();

	// нужно бы проверить, что стоит колбек на эвенты...
}


void map_access_acceptor::push_impl(
	const input_map_frame_params & params,
	const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	if (detail::tfdf_header_t::full_size >= tfdf_buffer_size)
	{
		// Этот фрейм не может быть валидным, так как в него максимум что влезает
		// это заголовок tfdf.
		_flush_accum(event_accepted_map_sdu::INCOMPLETE);
	}

	// Смотрим что там в tfdf заголовке
	detail::tfdf_header_t header;
	header.read(tfdf_buffer, true);
	const uint8_t * const tfdz_start = tfdf_buffer + header.size();
	const size_t tfdz_size = tfdf_buffer_size - header.size();

	// Если это MAP_SDU_START
	if (detail::tfdz_construction_rule_t::MAP_SDU_START == header.ctr_rule)
	{
		if (!_accumulator.empty())
			_flush_accum(event_accepted_map_sdu::INCOMPLETE);

		// Забираем пакет
		_consume_frame(params, header, tfdz_start, tfdz_size);
	}
	else if (detail::tfdz_construction_rule_t::MAP_SDU_CONTINUATION == header.ctr_rule)
	{
		// Если это продолжение фрейма
		if (_accumulator.empty())
			return; // Если мы пустые - однозначно выкидываем

		if (_prev_frame_seq_no.has_value() != params.frame_seq_no.has_value())
		{
			// Оп, это какая-то петрушка
			_flush_accum(event_accepted_map_sdu::INCOMPLETE);
			return;
		}

		// Если номера есть - сравним их
		if (_prev_frame_seq_no.has_value() && (*_prev_frame_seq_no + 1 != *params.frame_seq_no))
		{
			// Значит что-то где-то потерялось
			// Выкидываем и сбрасываем состояние
			_flush_accum(event_accepted_map_sdu::INCOMPLETE);
			return;
		}

		// Смотрим совпадает ли QOS
		if (_prev_frame_qos != params.qos)
		{
			// Выкидываем и сбрасываем состояние
			_flush_accum(event_accepted_map_sdu::INCOMPLETE);
			return;
		}

		// Если все ок - забираем фрейм
		_consume_frame(params, header, tfdz_start, tfdz_size);
	}
}


void map_access_acceptor::_consume_frame(
		const input_map_frame_params & params,
		const detail::tfdf_header_t & header,
		const uint8_t * tfdz, uint16_t tfdz_size
)
{
	assert(header.first_header_offset.has_value());

	// Если юнит не закончился в этом фрейме - в заголовке, в оффсете должно стоять 0xFFFF
	// Мы пойдем чуток дальше и будем считать что фрейм не закончился
	// Если в его оффсете стоит число большее, чем влезает в tfdz
	const uint32_t sdu_part_size = static_cast<uint32_t>(*header.first_header_offset) + 1;
	const bool sdu_complete = sdu_part_size <= tfdz_size;

	// Соответственно, если sdu уже закончился - берем только валидную его часть
	// Если нет - то берем весь tfdz
	uint16_t to_copy = sdu_complete
			? static_cast<uint16_t>(sdu_part_size)
			: tfdz_size
	;

	// Смотрим сколько мы можем скопировать в аккумулятор, чтобы его не переполнить
	bool overflow = false;
	const size_t next_step_accum_size = _accumulator.size() + to_copy;
	if (next_step_accum_size > _max_sdu_size)
	{
		// Ой, ой, у нас будет переполнение
		// Копируем что можем и отдаем пользователю как есть
		to_copy = _max_sdu_size - _accumulator.size();
		overflow = true;
	}

	// Копируем все что можем в аккумулятор
	_accumulator.reserve(_accumulator.size() + to_copy);
	std::copy(tfdz, tfdz + to_copy, std::back_inserter(_accumulator));

	// Запоминием параметры фрейма на будущее
	_prev_frame_qos = params.qos;
	_prev_frame_seq_no = params.frame_seq_no;

	// Если случилось переполнение или просто мы получили весь SDU
	// Сообщаем пользователю об этом событием
	if (overflow || sdu_complete)
		_flush_accum(overflow ? event_accepted_map_sdu::INCOMPLETE : 0);
}


void map_access_acceptor::_flush_accum(int event_flags)
{
	if (_accumulator.empty())
		return;

	// Сбрасываем номер предыдущего пакета
	_prev_frame_seq_no.reset();

	event_accepted_map_sdu event;
	event.data = std::move(_accumulator);
	event.qos = _prev_frame_qos;
	event.flags = event_flags | event_accepted_map_sdu::MAPA;
	emit_event(event);
}


}}
