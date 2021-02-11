#include <ccsds/uslp/input_stack.hpp>

#include <cassert>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


//! Реализация эвент хендлера, которая ничего не делает
/*! Используется по-умолчанию как альтенатива nullptr в указателе на эвент хендлер */
class _default_input_stack_event_handler: public input_stack_event_handler
{
protected:
	virtual void on_map_sdu_event(gmapid_t mapid, const event_accepted_map_sdu & evt) override
	{

	}
};


static _default_input_stack_event_handler _default_event_handler;


input_stack::input_stack()
{
	set_event_handler(&_default_event_handler);
}


void input_stack::set_event_handler(input_stack_event_handler * event_handler)
{
	_event_handler = event_handler;
}


void input_stack::push_frame(const uint8_t * frame_data, size_t frame_data_size)
{
	if (!_pchannel)
		throw einval_exception("unable to push data to input stack without pchannel in it");

	_pchannel->push_frame(frame_data, frame_data_size);
}


void input_stack::_event_callback(const gmapid_t & mapid, const event & evt)
{
	switch (evt.kind)
	{
	case event::kind_t::ACCEPTED_MAP_SDU:
		_event_handler->on_map_sdu_event(mapid, dynamic_cast<const event_accepted_map_sdu&>(evt));
		break;

	default:
		assert(false);
		break;
	}
}


void input_stack::_event_callback(const gvcid_t & gvcid, const event & evt)
{
	// Таких эвентов у нас пока нет
	assert(false);
}


void input_stack::_event_callback(const mcid_t & mcid, const event & evt)
{
	// Таких эвентов у нас пока нет
	assert(false);
}


void input_stack::_event_callback(const std::string & pchannel_id, const event & evt)
{
	// Таких эвентов у нас пока нет
	assert(false);
}


}}
