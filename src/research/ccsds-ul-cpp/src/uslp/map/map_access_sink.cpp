#include <ccsds/uslp/map/map_access_sink.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>


namespace ccsds { namespace uslp {


map_access_sink::map_access_sink(gmap_id_t mapid_)
		: map_sink(mapid_),
		  _prev_frame_qos(qos_t::EXPIDITED) // Просто чтобы не иметь UB
{

}


void map_access_sink::finalize_impl()
{
	map_sink::finalize_impl();

	// нужно бы проверить, что стоит колбек на эвенты...
}


void map_access_sink::push_impl(
	const input_map_frame_params & params,
	const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	// Смотрим что там в tfdf заголовке
	detail::tfdf_header_t header;
	header.read(tfdf_buffer, tfdf_buffer_size);

	// Если это MAP_SDU_START
	if (detail::tfdz_construction_rule_t::MAP_SDU_START == header.ctr_rule)
	{
		if (!_accumulator.empty())
		{
			// TODO: Какой-нибудь эвент
			_accumulator.clear();
		}

		// Забираем пакет
		consume_frame(params, header, tfdf_buffer + header.size(), tfdf_buffer_size - header.size());
	}
	else if (detail::tfdz_construction_rule_t::MAP_SDU_CONTINUATION == header.ctr_rule)
	{
		// Если это продолжение фрейма
		if (_accumulator.empty())
			return; // Если мы пустые - однозначно выкидываем

		// Смотрим совпадает ли номер фрейма
		if (_prev_frame_seq_no + 1 != params.frame_seq_no)
		{
			// Выкидываем и сбрасываем состояние
			_accumulator.clear();
			return;
		}

		// Смотрим совпадает ли QOS
		if (_prev_frame_qos != params.qos)
		{
			// Выкидываем и сбрасываем состояние
			_accumulator.clear();
			return;
		}

		// Если все ок - забираем фрейм
		consume_frame(params, header, tfdf_buffer + header.size(), tfdf_buffer_size - header.size());
	}
}


void map_access_sink::consume_frame(
		const input_map_frame_params & params,
		const detail::tfdf_header_t & header,
		const uint8_t * tfdz, uint16_t tfdz_size
)
{
	const bool unit_ended = *header.first_header_offset == 0xFFFF;

	uint16_t to_copy = unit_ended  ? tfdz_size : *header.first_header_offset + 1;
	_accumulator.resize(_accumulator.size() + to_copy);;
	std::copy(tfdz, tfdz + to_copy, std::back_inserter(_accumulator));

	// Если пейлоад закончился, то передаем его пользователю
	if (unit_ended)
	{
		map_sink_event_data_unit event(std::move(_accumulator), params.qos);
		_accumulator.clear();
		emit_event(event);

		// И на этом закончили...
		return;
	}

	// Запоминием параметры пакета
	_prev_frame_qos = params.qos;
	_prev_frame_seq_no = params.frame_seq_no;
}


}}
