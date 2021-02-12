#include <ccsds/uslp/physical/abstract_pchannel.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/_detail/tf_header.hpp>
#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


template<typename T>
static void _default_event_callback(const T &)
{

}



pchannel_emitter::pchannel_emitter(std::string name_)
		: channel_id(name_), _frame_version_no(detail::tf_header_t::default_frame_version_no)
{
	set_event_callback(_default_event_callback<emitter_event>);
}


void pchannel_emitter::frame_version_no(uint8_t value)
{
	if (value > 0x0f)
	{
		std::stringstream error;
		error << "Invalid value for frame version no: " << value << ". "
				<< "Value should be in range [0, 0x0F].";
		throw einval_exception(error.str());
	}

	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use frame_version_no() on pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	_frame_version_no = value;
}


void pchannel_emitter::frame_size(size_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use frame_size() on pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	_frame_size = value;
}


void pchannel_emitter::error_control_len(error_control_len_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use error_control_len() for pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	_error_control_len = value;
}


void pchannel_emitter::set_event_callback(event_callback_t event_callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callback() on pchannel source because pchannel is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(event_callback);
}


void pchannel_emitter::add_mchannel_source(mchannel_emitter * source)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use add_mchannel_source() on pchannel source because pchannel is finalized";
		throw object_is_finalized(error.str());
	}

	add_mchannel_source_impl(source);
}


void pchannel_emitter::finalize()
{
	if (_finalized)
		return;

	finalize_impl();

	// Если выше будет бросок - мы до сюда не дойдем
	_finalized = true;
}


bool pchannel_emitter::peek()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	return peek_impl();
}


bool pchannel_emitter::peek(pchannel_frame_params_t & frame_params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	return peek_impl(frame_params);
}


void pchannel_emitter::pop(uint8_t * frame_buffer, size_t frame_buffer_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use pop_frame() on pchannel source because it is finalized";
		throw object_is_finalized(error.str());
	}

	pchannel_frame_params_t frame_params;
	peek(frame_params);

	assert(frame_size() <= frame_buffer_size);
	pop_impl(frame_buffer);

	for (const auto & cookie: frame_params.payload_cookies)
	{
		emitter_event_sdu_emitted event;
		event.payload_cookie = cookie;
		emit_event(event);
	}
}


uint16_t pchannel_emitter::frame_size_overhead() const
{
	uint16_t retval = 0;

	switch (error_control_len())
	{
	case error_control_len_t::FOUR_OCTETS:
		retval += 4;
		break;

	case error_control_len_t::TWO_OCTETS:
		retval += 2;
		break;

	case error_control_len_t::ZERO:
		retval += 0;
		break;

	default:
		assert(false);
	};

	// TODO: Еще что-нибудь про инсерт зону

	return retval;
}


void pchannel_emitter::emit_event(const emitter_event & evt)
{
	_event_callback(evt);
}


void pchannel_emitter::finalize_impl()
{
	// Проверяем самые базовые настройки

	// Смотрим сколько нам нужно минимально места на кадр
	// Прям совсем минимально.
	// Только чтобы влез заголовок и только
	const size_t minimum_frame_size = frame_size_overhead();
	// Максимально влезает только так - больше в заголовок не поместится
	const size_t maximum_frame_size = 0x10000;

	if (_frame_size < minimum_frame_size || _frame_size > maximum_frame_size)
	{
		std::stringstream error;
		error << "invalid value for frame_size: " << _frame_size << ". "
				<< "Value should be in range [" << minimum_frame_size << ", " << maximum_frame_size << "]";
		throw einval_exception(error.str());
	}
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

pchannel_acceptor::pchannel_acceptor(std::string name_)
	: channel_id(std::move(name_))
{
	set_event_callback(_default_event_callback<acceptor_event>);
}


void pchannel_acceptor::insert_zone_size(uint16_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use insert_zone_size() on pchannel because it is finalized";
		throw object_is_finalized(error.str());
	}

	_insert_zone_size = value;
}


void pchannel_acceptor::error_control_len(error_control_len_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use error_control_len() on pchannel sink because it is finalized";
		throw object_is_finalized(error.str());
	}

	_error_control_len = value;
}


void pchannel_acceptor::set_event_callback(event_callback_t event_callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callback() on pchannel sink because it is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(event_callback);
}


void pchannel_acceptor::add_mchannel_acceptor(mchannel_acceptor * sink)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use add_mchannel_sink() on pchannel sink because it is finalized";
		throw object_is_finalized(error.str());
	}

	add_mchannel_acceptor_impl(sink);
}


void pchannel_acceptor::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


void pchannel_acceptor::push_frame(const uint8_t * frame_buffer, size_t frame_buffer_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use push_frame() on pchannel sink because it is not finalized";
		throw object_is_finalized(error.str());
	}

	push_frame_impl(frame_buffer, frame_buffer_size);
}


void pchannel_acceptor::emit_event(const acceptor_event & evt)
{
	_event_callback(evt);
}


void pchannel_acceptor::finalize_impl()
{
	// Ничего не делаем!
}



}}

