#include <ccsds/sdl/tc/pchannel/mchannel_rr_multiplexor.hpp>

#include <algorithm>
#include <cassert>


namespace ccsds { namespace sdl { namespace tc {


	void mchannel_rr_multiplexor::attach(mchannel_provider * provider)
	{
		_providers.push_back(provider);
		provider->max_frame_size(this->pchannel_adapter::max_buffer_size());
	}


	bool mchannel_rr_multiplexor::peek_frame(frame_params & fparams)
	{
		if (!_selected_provider)
			_selected_provider = _select_provider();

		if (!_selected_provider)
			return false;

		return _selected_provider->peek_frame(fparams);
	}


	void mchannel_rr_multiplexor::pop_frame(uint8_t * buffer)
	{
		assert(_selected_provider);
		try
		{
			_selected_provider->pop_frame(buffer);
			_selected_provider = nullptr;
		}
		catch (...)
		{
			_selected_provider = nullptr;
			throw;
		}
	}


	void mchannel_rr_multiplexor::pop_frame()
	{
		if (!_select_provider())
			return;

		try
		{
			_selected_provider->pop_frame();
			_selected_provider = nullptr;
		}
		catch (...)
		{
			_selected_provider = nullptr;
			throw;
		}
	}


	void mchannel_rr_multiplexor::max_buffer_size(size_t value)
	{
		pchannel_adapter::max_buffer_size(value);

		for (auto & provider: _providers)
			provider->max_frame_size(value);
	}


	mchannel_provider * mchannel_rr_multiplexor::_select_provider()
	{
		frame_params dummy_fparams;

		auto itt = _rr_iterator;
		const size_t iteration_limit = _providers.size();
		auto retval = _providers.end();

		// Проходим по всем провайдерам которые есть начиная с того на котором
		// закончили в прошлый раз и ищем первого который готов нам что-то сказать
		for (size_t i = 0; i < iteration_limit; i++, itt++)
		{
			if (itt == _providers.end())
				itt = _providers.begin();

			if ((*itt)->peek_frame(dummy_fparams))
			{
				retval = itt;
				break;
			}
		}

		if (retval != _providers.end())
		{
			// Если мы нашли кого-то готового - берем его
			// а rr итератор ставим на следующий
			_rr_iterator = std::next(itt, 1);
			return *itt;
		}

		// Если никого не нашли...
		// Не трогаем _rr_iterator - попробуем в другой раз
		return nullptr;
	}

}}}
