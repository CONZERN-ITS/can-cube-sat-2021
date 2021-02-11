#include <ccsds/uslp/map/map_access_emitter.hpp>

#include <sstream>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/idle_pattern.hpp>
#include <ccsds/uslp/_detail/tfdf_header.hpp>


namespace ccsds { namespace uslp {


map_access_emitter::map_access_emitter(gmapid_t map_id_)
	: map_emitter(map_id_)
{

}


void map_access_emitter::add_sdu(const uint8_t * data, size_t data_size, qos_t qos)
{
	data_unit_t du;
	std::copy(data, data + data_size, std::back_inserter(du.data));
	du.data_original_size = data_size;
	du.qos = qos;

	_data_queue.push_back(du);
}


void map_access_emitter::finalize_impl()
{
	// Убеждаемся что в tfdf влезает заголовок и хотябы один байтик информации
	if (detail::tfdf_header_t::full_size >= tfdf_size())
	{
		std::stringstream error;
		error << "tfdf zone is too small for map channel";
		throw einval_exception(error.str());
	}
}


bool map_access_emitter::peek_tfdf_impl()
{
	return !_data_queue.empty();
}


bool map_access_emitter::peek_tfdf_impl(output_map_frame_params & params)
{
	// Если в очереди нет ничего готового на отправку - то и забьем
	if (_data_queue.empty())
		return false;

	params.payload_cookies.clear();

	const auto & front_elem = _data_queue.front();
	params.qos = front_elem.qos;

	const size_t payload_max_size = _tfdz_size();
	const size_t available_size = _data_queue.front().data.size();
	if (available_size > payload_max_size)
	{
		// Если то, что есть в очереди мы не сможем отправить одним фреймом
		// Придется чуток занять канал
		params.channel_lock = available_size > payload_max_size;
	}
	else
	{
		// Если следующий фрейм вмещает последние байты нашего SDU
		// Покрасим этот SDU кукисом, чтобы получить о нем отчеты дальше по стеку
		params.payload_cookies.push_back(front_elem.cookie);
	}
	return true;
}


void map_access_emitter::pop_tfdf_impl(uint8_t * tfdf_buffer)
{
	if (_data_queue.empty())
		return;

	// О, что-то у нас да есть
	auto & data_unit = _data_queue.front();

	// Отступаем под заголовок
	uint8_t * const tfdz_buffer = tfdf_buffer + detail::tfdf_header_t::full_size;
	const uint16_t tfdz_buffer_size = map_emitter::tfdf_size() - detail::tfdf_header_t::full_size;
	// Сохраняем начало буфера - потом по этому началу мы посчитаем оффсет для заголовка

	// Вываливаем пейлоад
	const bool element_begun = data_unit.data.size() == data_unit.data_original_size;

	uint16_t to_copy_size = static_cast<uint16_t>(
		std::min(
			static_cast<size_t>(tfdz_buffer_size),
			data_unit.data.size()
		)
	);

	// Копируем!
	auto to_copy_begin = std::cbegin(data_unit.data);
	auto to_copy_end = std::next(to_copy_begin, to_copy_size);
	std::copy(to_copy_begin, to_copy_end, tfdz_buffer);

	// Откусываем скопированное
	bool element_ended = false;
	data_unit.data.erase(to_copy_begin, to_copy_end);
	if (data_unit.data.empty())
	{
		element_ended = true;
		_data_queue.pop_front();
	}

	// Пишем заголовок
	detail::tfdf_header_t header;
	header.upid = static_cast<int>(detail::upid_t::MAP_ACCESS_SDU);

	// Если элемент только начался - показываем это специальным флагом
	header.ctr_rule = static_cast<int>(
			element_begun
				? detail::tfdz_construction_rule_t::MAP_SDU_START
				: detail::tfdz_construction_rule_t::MAP_SDU_CONTINUATION
	);

	// Если элемент закончился в этом фрейме - покажем последний его валидный байт
	if (element_ended)
	{
		header.first_header_offset = to_copy_size - 1;

		// И забьем лишние данные айдлами
		const auto & idle_generator = idle_pattern_generator::instance();
		std::copy(
				idle_generator.begin(),
				idle_generator.end(tfdz_buffer_size - to_copy_size),
				tfdz_buffer + to_copy_size
		);
	}
	else
	{
		header.first_header_offset = 0xFFFF; // Если нет - покажем, что он не кончился в этом фрейме
	}

	// Все посчитали - теперь пишем заголовок
	header.write(tfdf_buffer);

	// Готово!
}


uint16_t map_access_emitter::_tfdz_size() const
{
	// У нас всегда полнознамерные заголовки
	return tfdf_size() - detail::tfdf_header_t::full_size;
}


}}

