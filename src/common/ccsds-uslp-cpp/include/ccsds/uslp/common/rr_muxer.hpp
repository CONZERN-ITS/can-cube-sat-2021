#ifndef INCLUDE_CCSDS_USLP_COMMON_RR_MUXER_HPP_
#define INCLUDE_CCSDS_USLP_COMMON_RR_MUXER_HPP_


#include <cstddef>
#include <algorithm>
#include <vector>


namespace ccsds { namespace uslp {


template<typename MUXED_SOURCE_T, typename MUXED_SOURCE_STATUS_CHECKER_T>
class rr_muxer
{
public:
	typedef MUXED_SOURCE_T muxed_source_t;
	typedef MUXED_SOURCE_STATUS_CHECKER_T muxed_source_status_checker_t;

private:
	typedef std::vector<muxed_source_t> container_t;

public:
	rr_muxer(): _next_candidate(_sources.end()) {}
	virtual ~rr_muxer() = default;

	typedef typename container_t::iterator iterator;
	typedef typename container_t::const_iterator const_iterator;

	const_iterator cbegin() const { return _sources.begin(); }
	const_iterator begin() const { return _sources.begin(); }
	iterator begin() { return _sources.begin(); }

	const_iterator cend() const { return _sources.end(); }
	const_iterator end() const { return _sources.end(); }
	iterator end() { return _sources.end(); }


	void reset()
	{
		_next_candidate = _sources.begin();
	}


	void add_source(muxed_source_t source)
	{
		_sources.push_back(source);
		// Здесь нужно сделать сброс, так как после добавления элемента в вектор
		// итераторы могут инвалидироваться
		reset();
	}


	muxed_source_t select_next()
	{
		const auto iteration_limit = _sources.size();
		auto itt = _next_candidate;
		auto selected = _sources.end();

		typename std::decay<decltype(iteration_limit)>::type i;
		for (i = 0; i < iteration_limit; i++, itt++)
		{
			// Проверяем на вывалились ли мы за пределы контейнера
			if (_sources.end() == itt)
				itt = _sources.begin();

			// Смотрим готов ли этот источник предоставить данные
			if (_status_checker(*itt))
			{
				selected = itt;
				break;
			}

			// Если не готов, продолжим искать дальше
		}

		if (selected != _sources.end())
		{
			// Если мы нашли кого-то, кто готов предоставить данные
			// Запоминаем, чтобы в следующий раз опросить следующего
			_next_candidate = std::next(selected, 1);
			// Возвращаем результат
			return *selected;
		}

		// Если никого не нашли готового - так и скажем
		// Внутренние каретки при этом не трогаем
		return nullptr;
	}


private:
	typename container_t::iterator _next_candidate;
	muxed_source_status_checker_t _status_checker;
	container_t _sources;
};


}}



#endif /* INCLUDE_CCSDS_USLP_COMMON_RR_MUXER_HPP_ */
