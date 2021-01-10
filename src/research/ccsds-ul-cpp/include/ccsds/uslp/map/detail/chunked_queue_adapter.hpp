#ifndef INCLUDE_CCSDS_USLP_MAP_CHUNKED_READER_HPP_
#define INCLUDE_CCSDS_USLP_MAP_CHUNKED_READER_HPP_

#include <cstdint>
#include <cstddef>
#include <algorithm>


namespace ccsds { namespace uslp { namespace detail {

template<typename CONTAINER_T>
//! Вспомогательный класс, который позволяет читать данные из очереди произвольным кусками
/*! Элемент очереди должен иметь методы с сигнатуами
	const_iterator begin() const;
	const_iterator end() const;

	Итераторы, наверное, могут быть хоть форвард итераторами
*/
class chunked_queue_adadpter
{
public:
	//! Тип объекта очерди на который мы садимся
	typedef CONTAINER_T container_type;
	//! Тип элемента, который мы будем доставать из очереди
	typedef typename container_type::value_type value_type;

	//! Конструктор. В параметрах указывается элемент, который мы будем читать
	chunked_queue_adadpter(container_type & the_deque)
		: _the_deque(the_deque)
	{}

	//! Метод возвращает размер куска, который мы можем прочесть из первого элемента очереди
	/*! Важно что мы читаем только из текущего элемента очереди. Если в указанном элементе осталось
		30 непрочитанных байт, а в последующих еще 100500, то этот метод вернет 30 */
	std::tuple<bool, size_t> peek_chunk() const
	{
		// Если в очереди ничего нет - прочесть ничего и не можем
		if (_the_deque.empty())
			return std::make_tuple(false, 0);

		// Если мы работаем с текущим элементом - то его каретка у нас есть
		// Для прочих элементов каретка всегда нулями

		// Если в очереди что-то всетаки есть
		const auto & elem = _the_deque.front();
		auto chunk_begin_itt = std::next(std::cbegin(elem), _front_elem_carret);

		bool elem_begun = 0 == _front_elem_carret;
		size_t chunk_size = std::distance(chunk_begin_itt, std::cend(elem));

		return std::make_tuple(elem_begun, chunk_size);
	}


	// Этот метод показывает сколько вообще всего байт готово для отправки
	std::tuple<bool, size_t> peek_all() const
	{
		size_t retval = 0;
		for (auto itt = std::cbegin(_the_deque); itt != std::cend(_the_deque); itt++)
			retval += std::distance(itt->begin(), itt->end());

		auto at_elem_begin = 0 == _front_elem_carret;
		return std::make_tuple(at_elem_begin, retval);
	}


	std::tuple<bool, size_t, bool> pop_chunk(uint8_t * dest, size_t chunk_size)
	{
		if (_the_deque.empty())
			return std::make_tuple(false, 0, false);

		const auto & elem = _the_deque.front();
		const auto chunk_begin_itt = std::next(std::cbegin(elem), _front_elem_carret);
		// Если мы только начали читать чанк - сообщим об этом
		bool elem_just_begun = 0 == _front_elem_carret;

		// Считаем сколько мы можем по факту скопировать
		const auto available = static_cast<size_t>(std::distance(chunk_begin_itt, std::cend(elem)));
		chunk_size = std::min(chunk_size, available);
		const auto chunk_end_itt = std::next(chunk_begin_itt, chunk_size);
		// Копируем!
		std::copy(chunk_begin_itt, chunk_end_itt, dest);

		bool elem_just_ended = false;
		// Если мы прочли весь элемент
		if (std::cend(elem) == chunk_end_itt)
		{
			// Дропаем его
			elem_just_ended = true;
			_the_deque.pop_front();
			_front_elem_carret = 0;
		}
		else
		{
			// Сдвигаем каретку на прочитанное
			_front_elem_carret += chunk_size;
		}

		return std::tie(elem_just_begun, chunk_size, elem_just_ended);
	}

private:
	size_t _front_elem_carret = 0;
	container_type & _the_deque;
};

}}}



#endif /* INCLUDE_CCSDS_USLP_MAP_CHUNKED_READER_HPP_ */
