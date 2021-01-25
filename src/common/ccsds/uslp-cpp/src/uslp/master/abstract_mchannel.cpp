#include <ccsds/uslp/master/abstract_mchannel.hpp>

#include <sstream>
#include <cstring>

#include <endian.h>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/_detail/tf_header.hpp>


namespace ccsds { namespace uslp {


mchannel_source::mchannel_source(mcid_t mcid_)
	: mcid(mcid_)
{

}


void mchannel_source::frame_du_size_l1(uint16_t value)
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

	if (source->gvcid.mcid() != this->mcid)
	{
		std::stringstream error;
		error << "invalid gvcid sink "
				<< static_cast<int>(source->gvcid.sc_id())
				<< "," << static_cast<int>(source->gvcid.vchannel_id())
				<< " for vchannel attached to mchannel sink "
				<< static_cast<int>(this->mcid.sc_id())
		;

		throw einval_exception(error.str());
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

	finalize_impl();
	_finalized = true;
}


bool mchannel_source::peek_frame_du()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_du_impl();
}


bool mchannel_source::peek_frame_du(mchannel_frame_params_t & frame_params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame(frame_params) on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_frame_du_impl(frame_params);
}


void mchannel_source::pop_frame_du(uint8_t * frame_data_unit_buffer, uint16_t frame_data_unit_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use pop_frame() on mchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	// Нам нужны параметры фрейма, чтобы прикинуть где будет OCF поле
	mchannel_frame_params_t params;
	peek_frame_du(params);

	// Выгружаем данные фрейма
	pop_frame_du_impl(frame_data_unit_buffer);

	// Теперь OCF поле
	if (_ocf_source != nullptr)
	{
		// Считаем отступ от TFDF поля до OCF поля
		// У нас есть размер фрейма с заголовками и OCF
		// Значит нужно из этого размера вычесть размер заголовков и OCF
		// и мы получим размер TFDF. Сразу после него и лежит OCF
		const uint16_t ocf_tfdf_offset = frame_du_size_l1()
				- detail::tf_header_t::extended_size_forecast(params.frame_seq_no)
				- sizeof(uint32_t)
		;

		uint8_t * const ocf_location = frame_data_unit_buffer + ocf_tfdf_offset;

		// Пишем OCF по адресу
		uint32_t word = _ocf_source->get_ocf();
		word = htobe32(word);
		std::memcpy(ocf_location, &word, sizeof(word));
	}
}


uint16_t mchannel_source::frame_size_overhead() const
{
	// Если у нас есть OCF поле - то его мы должны уметь вытаскивать
	uint16_t retval = 0;
	if (_ocf_source)
		retval += sizeof(uint32_t);

	return retval;
}


void mchannel_source::finalize_impl()
{
	// Единственное что мы тут можем проверить - размер фрейма. Что он не меньше чем должен быть
	// Минимальный размер - чтобы влезал заголовок

	auto min_frame_size = frame_size_overhead();
	if (frame_du_size_l1() < min_frame_size)
	{
		std::stringstream error;
		error << "invalid frame size l1 on mchannel: " << frame_du_size_l1() << ". "
				<< "It should be no less than " << min_frame_size;
		throw einval_exception(error.str());
	}
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


mchannel_sink::mchannel_sink(mcid_t mcid_)
	: mcid(mcid_)
{

}


void mchannel_sink::add_vchannel_sink(vchannel_sink * sink)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use add_vchannel_sink(sink) on physical channel because it is finalized";
		throw object_is_finalized(error.str());
	}

	if (sink->gvcid.mcid() != this->mcid)
	{
		std::stringstream error;
		error << "invalid gvcid sink "
				<< static_cast<int>(sink->gvcid.sc_id())
				<< "," << static_cast<int>(sink->gvcid.vchannel_id())
				<< " for vchannel attached to mchannel source "
				<< static_cast<int>(this->mcid.sc_id())
		;

		throw einval_exception(error.str());
	}

	add_vchannel_sink_impl(sink);
}


void mchannel_sink::set_ocf_sink(ocf_sink * sink)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_ocf_sink(sink) on physical channel because it is finalized";
		throw object_is_finalized(error.str());
	}

	_ocf_sink = sink;
}


void mchannel_sink::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


void mchannel_sink::push(
		const mchannel_frame_params_t & frame_params,
		const uint8_t * frame_data_unit, uint16_t frame_data_unit_size
)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use push(frame_reader) on physical channel because it is finalized";
		throw object_is_finalized(error.str());
	}

	push_impl(frame_params, frame_data_unit, frame_data_unit_size);

	// Если в этом фрейме есть OCF и у нас есть кому его передавать - мы его возьмем
	if (frame_params.ocf_present && _ocf_sink != nullptr)
	{
		uint32_t word;
		const uint8_t * ocf_location = frame_data_unit + frame_data_unit_size - sizeof(uint32_t);
		std::memcpy(&word, ocf_location, sizeof(uint32_t));
		word = htobe32(word);
		_ocf_sink->put_ocf(word);
	}
}


void mchannel_sink::finalize_impl()
{
	// Ничего не делаем!
}



}}
