#include <ccsds/uslp/map/detail/tfdf_header.hpp>
#include <ccsds/uslp/map/map_access_service.hpp>



namespace ccsds { namespace uslp {


map_access_service::map_access_service(gmap_id_t map_id_)
	: map_service(map_id_), _chunk_reader(_data_queue)
{
	// С порога ставим себе размер зоны, чтобы в нее хоть заголовок влез
	tfdf_size(detail::tfdf_header::full_size);
}


bool map_access_service::peek_tfdf()
{
	return !_data_queue.empty();
}


bool map_access_service::peek_tfdf(tfdf_params & params)
{
	// Если в очереди нет ничего готового на отправку - то и забьем
	if (_data_queue.empty())
		return false;

	const auto & front_elem = _data_queue.front();
	params.qos = front_elem.qos;

	bool elem_begun;
	size_t readable;
	std::tie(elem_begun, readable) = _chunk_reader.peek_chunk();
	// Если то, что есть в очереди мы не сможем отправить одним фреймом
	// Придется чуток занять канал
	params.channel_lock = readable > (tfdf_size() - detail::tfdf_header::full_size);

	return true;
}


void map_access_service::pop_tfdf(uint8_t * tfdf_buffer)
{
	if (!peek_tfdf())
		return;

	// Отступаем под заголовок
	auto * payload_begin = tfdf_buffer + detail::tfdf_header::full_size;
	auto payload_max_size = tfdf_size() - detail::tfdf_header::full_size;

	// Вываливаем пейлоад
	size_t chunk_size;
	bool elem_begun;
	bool elem_ended;
	std::tie(elem_begun, chunk_size, elem_ended) = _chunk_reader.pop_chunk(payload_begin, payload_max_size);

	// Пишем заголовок
	detail::tfdf_header header;
	header.upid = static_cast<int>(detail::upid_t::MAP_ACCESS_SDU);

	// Если элемент только начался - показываем это специальным флагом
	header.ctr_rule = static_cast<int>(
			elem_begun
				? detail::tfdz_construction_rule_t::MAP_SDU_START
				: detail::tfdz_construction_rule_t::MAD_SDU_CONTINUATION
	);

	// Если элемент закончился в этом фрейме - покажем последний его валидный байт
	if (elem_ended)
		header.first_header_offset = chunk_size - 1;
	else
		header.first_header_offset = 0xFFFF; // Если нет - покажем, что он не кончился в этом фрейме


	// Все посчитали - теперь пишем
	header.write(tfdf_buffer);
	// На этом все!
}

}}

