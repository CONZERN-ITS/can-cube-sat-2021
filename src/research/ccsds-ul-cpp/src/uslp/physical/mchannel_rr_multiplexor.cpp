#include <ccsds/uslp/physical/mchannel_rr_multiplexor.hpp>

#include <cassert>

namespace ccsds { namespace uslp {


mchannel_rr_multiplexor::mchannel_rr_multiplexor(std::string name)
	: pchannel_service(std::move(name))
{
	_rr_iterator = _mchannels.end();
}


void mchannel_rr_multiplexor::frame_size(int32_t value)
{
	// Передаем эту опцию мультеплексируемым каналалам
	for (auto itt = _mchannels.begin(); itt != _mchannels.end(); itt++)
		itt->second->max_frame_size(this->pchannel_service::frame_size());
}


bool mchannel_rr_multiplexor::peek_frame()
{
	if (!_selected_mchannel)
		_selected_mchannel = _select_next_to_pop();

	if (!_selected_mchannel)
		return false;

	return _selected_mchannel->peek_frame();
}


bool mchannel_rr_multiplexor::peek_frame(pchannel_frame_params_t & frame_params)
{
	if (!_selected_mchannel)
		_selected_mchannel = _select_next_to_pop();

	if (!_selected_mchannel)
		return false;

	mchannel_frame_params_t dummy;
	bool rv = _selected_mchannel->peek_frame(dummy);
	if (!rv)
		return false;

	// frame_params.data_field_size = dummy.data_field_size ; // TODO Полное преобразование
	return true;
}


void mchannel_rr_multiplexor::pop_frame(uint8_t * frame_buffer)
{
	assert(_selected_mchannel);
	_selected_mchannel->pop_frame(frame_buffer);
}


void mchannel_rr_multiplexor::pop_frame()
{
	assert(_selected_mchannel);
	_selected_mchannel->pop_frame();
}


void mchannel_rr_multiplexor::push_frame(uint8_t * frame_buffer)
{
	// TODO: Разобрать заголовки...
}


void mchannel_rr_multiplexor::attach_master_channel_service(mchannel_service * mchannel)
{
	this->_mchannels.insert(std::make_pair(mchannel->mcid, mchannel));
	mchannel->max_frame_size(this->pchannel_service::frame_size());
}


mchannel_service * mchannel_rr_multiplexor::_select_next_to_pop()
{
	auto itt = _rr_iterator;
	size_t iteration_limit = _mchannels.size();
	auto found = _mchannels.end();

	for (size_t i = 0; i < iteration_limit; i++)
	{
		if (itt == _mchannels.end())
			itt = _mchannels.begin();

		if (itt->second->peek_frame())
		{
			// Ага, нашли кого-то кто готов давать данные
			found = itt;
			break;
		}
	}

	if (found == _mchannels.end())
	{
		// Никого не нашли, сдаемся
		return nullptr;
	}

	// Кого-то да нашли
	// Переставляем rr итератор на следующий канал
	// (он точно не end(), мы уже проверили
	_rr_iterator = std::next(found, 1);
	return found->second;
}



}}



