#include <ccsds/uslp/master/abstract_mchannel.hpp>

#include <sstream>

#include <ccsds/uslp/_detail/tf_header.hpp>
#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


mchannel_source::mchannel_source(mcid_t mcid_)
	: mcid(mcid_)
{

}


void mchannel_source::frame_size_l1(uint16_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to set frame_size_l1 on mchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_frame_size_l1 = value;
}


void mchannel_source::id_is_destination(bool value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use id_is_destination on mchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_id_is_destination = value;
}


void mchannel_source::add_vchannel_source(vchannel_source * source)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to set use add_vchannel_source() on mchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	add_vchannel_source_impl(source);
}


void mchannel_source::set_ocf_source(ocf_source * source)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_ocf_source() on mchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_ocf_source = source;
}


void mchannel_source::finalize()
{
	if (_finalized)
		return;

	check_and_sync_config();
	_finalized = true;
}


bool mchannel_source::peek_frame()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_impl();
}


bool mchannel_source::peek_frame(mchannel_frame_params_t & frame_params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame(frame_params) on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_impl(frame_params);
}


void mchannel_source::pop_frame(uint8_t * tfdf_buffer)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use pop_frame() on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	pop_frame_impl(tfdf_buffer);
}


uint16_t mchannel_source::frame_size_overhead() const
{
	// Если у нас есть OCF поле - то его мы должны уметь вытаскивать
	uint16_t retval = 0;
	if (_ocf_source)
		retval += sizeof(uint32_t);

	return retval;
}


void mchannel_source::check_and_sync_config()
{
	// Единственное что мы тут можем проверить - размер фрейма. Что он не меньше чем должен быть
	// Минимальный размер - чтобы влезал заголовок

	auto min_frame_size = frame_size_overhead();
	if (frame_size_l1() < min_frame_size)
	{
		std::stringstream error;
		error << "invalid frame size l1 on mchannel: " << frame_size_l1() << ". "
				<< "It should be no less than " << min_frame_size;
		throw einval_exception(error.str());
	}

}



}}
