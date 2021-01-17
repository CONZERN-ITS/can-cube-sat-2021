#include <ccsds/uslp/physical/abstract_pchannel.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/_detail/tf_header.hpp>
#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


pchannel_source::pchannel_source(std::string name_)
		: name(name_)
{

}


void pchannel_source::frame_size(int32_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to set frame size for phys channel because it is already finalized";
		throw object_is_finalized(error.str());
	}

	_frame_size = value;
}


void pchannel_source::error_control_len(error_control_len_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to set error_control_len for phys channel because it is already finalized";
		throw object_is_finalized(error.str());
	}

	_error_control_len = value;
}


void pchannel_source::add_mchannel_source(mchannel_source * source)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to add mchannel source to pchannel because pchannel is finalized";
		throw object_is_finalized(error.str());
	}

	add_mchannel_source_impl(source);
}


void pchannel_source::finalize()
{
	if (_finalized)
		return;

	check_and_sync_config();

	// Если выше будет бросок - мы до сюда не дойдем
	_finalized = true;
}


bool pchannel_source::peek_frame()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on virtual channel before it is finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_impl();
}


bool pchannel_source::peek_frame(pchannel_frame_params_t & frame_params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame(frame_params) on physical channel before it is finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_impl(frame_params);
}


void pchannel_source::pop_frame(uint8_t * frame_buffer)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use pop_frame() on physical channel before it is finalized";
		throw object_is_finalized(error.str());
	}

	pop_frame_impl(frame_buffer);
}


uint16_t pchannel_source::frame_size_overhead() const
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

	return retval;
}


void pchannel_source::check_and_sync_config()
{
	// Проверяем самые базовые настройки

	// Смотрим сколько нам нужно минимально места на кадр
	// Прям совсем минимально.
	// Только чтобы влез заголовок и только
	const int32_t minimum_frame_size = frame_size_overhead();
	// Максимально влезает только так - больше в заголовок не поместится
	const int32_t maximum_frame_size = 0x10000;

	if (_frame_size < minimum_frame_size || _frame_size > maximum_frame_size)
	{
		std::stringstream error;
		error << "invalid value for frame_size: " << _frame_size << ". "
				<< "Value should be in range [" << minimum_frame_size << ", " << maximum_frame_size << "]";
		throw einval_exception(error.str());
	}

}


}}

