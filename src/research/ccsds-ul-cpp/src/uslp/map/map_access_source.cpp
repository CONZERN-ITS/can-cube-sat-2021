#include <ccsds/uslp/exceptions.hpp>

#include <sstream>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>
#include <ccsds/uslp/map/map_access_source.hpp>


namespace ccsds { namespace uslp {


map_access_source::map_access_source(gmap_id_t map_id_)
	: map_source(map_id_)
{

}


void map_access_source::add_sdu(const uint8_t * data, size_t data_size, qos_t qos)
{
	data_unit_t du;
	std::copy(data, data + data_size, std::back_inserter(du.data));
	du.data_original_size = data_size;
	du.qos = qos;

	_data_queue.push_back(du);
}


void map_access_source::check_and_sync_config()
{
	// Убеждаемся что в tfdf влезает заголовок и хотябы один байтик информации
	if (detail::tfdf_header::full_size >= tfdf_size())
	{
		std::stringstream error;
		error << "tfdf zone is too small for map channel";
		throw einval_exception(error.str());
	}
}


bool map_access_source::peek_tfdf_impl()
{
	return !_data_queue.empty();
}


bool map_access_source::peek_tfdf_impl(tfdf_params & params)
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


void map_access_source::pop_tfdf_impl(uint8_t * tfdf_buffer)
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


uint16_t map_access_source::_tfdz_size() const
{
	// У нас всегда полнознамерные заголовки
	return tfdf_size() - detail::tfdf_header::full_size;
}


}}
