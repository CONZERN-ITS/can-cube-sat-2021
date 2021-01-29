#include <ccsds/uslp/physical/mchannel_demuxer.hpp>

#include <ccsds/uslp/_detail/tf_header.hpp>

namespace ccsds { namespace uslp {


mchannel_demuxer::mchannel_demuxer(std::string name_)
		: pchannel_sink(std::move(name_))
{

}


void mchannel_demuxer::finalize_impl()
{
	pchannel_sink::finalize_impl();

	for (auto & pair: _container)
		pair.second->finalize();
}


void mchannel_demuxer::add_mchannel_sink_impl(mchannel_sink * sink)
{
	_container.insert(std::make_pair(sink->channel_id, sink));
}


void mchannel_demuxer::push_frame_impl(const uint8_t * frame_buffer, size_t frame_buffer_size)
{
	// Парсим фрейм!

	// разбираем заголовки
	detail::tf_header_t header;
	header.read(frame_buffer);

	// Если у фрейма нет полноценных заголовков - это не наш фрейм
	if (!header.ext)
		return; // дропаем его как есть


	// Разбираем инсерт зону
	const uint16_t insert_zone_size = pchannel_sink::insert_zone_size();
	// const uint8_t * const insert_zone_begin = frame_buffer + header.size();

	// TODO Делаем что-нибудь там с инсерт зоной

	// Разбираем поле данных (tfdf + ocf)
	const uint16_t frame_data_unit_size =
			frame_buffer_size - header.size() - insert_zone_size
			- static_cast<uint16_t>(error_control_len())
	;
	const uint8_t * const frame_data_unit_begin = frame_buffer + header.size() + insert_zone_size;

	// Разираем поле контрольной суммы
	//const uint16_t error_control_size = static_cast<uint16_t>(error_control_len());
	//const uint8_t * const error_control_begin =
	//		frame_buffer + frame_buffer_size
	//		- error_control_size
	//;

	// TODO: Проверяем контрольную сумму

	// Формируем параметры кадра
	mchannel_frame_params_t params;
	params.channel_id = header.gmap_id;
	params.id_is_destination = header.id_is_destination;
	params.frame_class = header.ext->frame_class;
	params.ocf_present = header.ext->ocf_present;
	params.frame_seq_no = header.ext->frame_seq_no;

	// Ищем канал, для которого предназначен этот кадр
	auto itt = _container.find(header.gmap_id);
	if (itt == _container.end())
	{
		// У нас таких нет! Странно конечно, но сливаемся
		return;
	}

	// Отдаем этажем выше
	itt->second->push(params, frame_data_unit_begin, frame_data_unit_size);
}


}}
