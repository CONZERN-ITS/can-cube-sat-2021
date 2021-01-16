#include <ccsds/uslp/exceptions.hpp>

#include <ccsds/uslp/_detail/tfdf_header.hpp>
#include <ccsds/uslp/map/map_access_source.hpp>


namespace ccsds { namespace uslp {


map_access_source::map_access_source(gmap_id_t map_id_)
	: map_source(map_id_), _data_queue()
{
	// С порога ставим себе размер зоны, чтобы в нее хоть заголовок влез
	tfdf_size(detail::tfdf_header::full_size);
}


void map_access_source::tfdf_size(uint16_t value)
{
	// Проверка на то, что мы можем столько
	if (value < detail::tfdf_header::full_size)
		throw einval_exception("map_access service requires no less than 4 bytes for tfdf zone size");

	this->map_source::tfdf_size(value);
}


bool map_access_source::peek_tfdf()
{
	return !_data_queue.empty();
}


bool map_access_source::peek_tfdf(tfdf_params & params)
{
	// Если в очереди нет ничего готового на отправку - то и забьем
	if (_data_queue.empty())
		return false;

	const auto & front_elem = _data_queue.front();
	params.qos = front_elem.qos;

	// Если то, что есть в очереди мы не сможем отправить одним фреймом
	// Придется чуток занять канал
	const size_t payload_max_size = map_source::tfdf_size() - detail::tfdf_header::full_size;
	const size_t available_size = _data_queue.front().data.size();
	params.channel_lock = available_size > payload_max_size;

	return true;
}


void map_access_source::pop_tfdf(uint8_t * tfdf_buffer)
{
	if (_data_queue.empty())
		return;

	// О, что-то у нас да есть
	auto & data_unit = _data_queue.front();

	// Отступаем под заголовок
	auto * output_buffer = tfdf_buffer + detail::tfdf_header::full_size;
	uint16_t output_buffer_size = map_source::tfdf_size() - detail::tfdf_header::full_size;
	const uint8_t * const tdfz_start = output_buffer;

	// Вываливаем пейлоад
	const bool element_begun = data_unit.data.size() == data_unit.data_original_size;

	uint16_t to_copy_size = static_cast<uint16_t>(
		std::min(
			static_cast<size_t>(output_buffer_size),
			data_unit.data.size()
		)
	);

	// Копируем!
	auto to_copy_begin = std::cbegin(data_unit.data);
	auto to_copy_end = std::next(to_copy_begin, to_copy_size);
	std::copy(to_copy_begin, to_copy_end, output_buffer);

	// Откусываем скопированное
	bool element_ended = false;
	data_unit.data.erase(to_copy_begin, to_copy_end);
	if (data_unit.data.empty())
	{
		element_ended = true;
		_data_queue.pop_front();
	}

	output_buffer += to_copy_size;
	output_buffer_size -= to_copy_size;

	// Пишем заголовок
	detail::tfdf_header header;
	header.upid = static_cast<int>(detail::upid_t::MAP_ACCESS_SDU);

	// Если элемент только начался - показываем это специальным флагом
	header.ctr_rule = static_cast<int>(
			element_begun
				? detail::tfdz_construction_rule_t::MAP_SDU_START
				: detail::tfdz_construction_rule_t::MAP_SDU_CONTINUATION
	);

	// Если элемент закончился в этом фрейме - покажем последний его валидный байт
	if (element_ended)
		header.first_header_offset = (output_buffer - tdfz_start) - 1;
	else
		header.first_header_offset = 0xFFFF; // Если нет - покажем, что он не кончился в этом фрейме

	// Все посчитали - теперь пишем
	header.write(tfdf_buffer);

	// Готово!
}

}}

