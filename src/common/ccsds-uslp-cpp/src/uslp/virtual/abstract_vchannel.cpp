#include <ccsds/uslp/virtual/abstract_vchannel.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/_detail/tf_header.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>


namespace ccsds { namespace uslp {


template<typename T>
static void _default_event_callback(const T &)
{

}


vchannel_emitter::vchannel_emitter(gvcid_t gvcid_)
	: channel_id(gvcid_)
{
	set_event_callback(_default_event_callback<emitter_event>);
}


void vchannel_emitter::tfdf_size(uint16_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use frame_size_l2 on vchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_frame_size_l2 = value;
}


void vchannel_emitter::frame_seq_no_len(uint8_t len)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use frame_seq_no_len on vchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_frame_seq_no.value(0, len);
}


void vchannel_emitter::set_event_callback(event_callback_t callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callback on vchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(callback);
}


void vchannel_emitter::add_map_source(map_emitter * source)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use add_map_source on vchannel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	if (source->channel_id.gvcid() != this->channel_id)
	{
		std::stringstream error;
		error << "invalid gmap id "
				<< static_cast<int>(source->channel_id.sc_id())
				<< "," << static_cast<int>(source->channel_id.vchannel_id())
				<< "," << static_cast<int>(source->channel_id.map_id())
				<< " for map channel attached to vchannel source"
				<< static_cast<int>(this->channel_id.sc_id())
				<< "," << static_cast<int>(this->channel_id.vchannel_id())
		;

		throw einval_exception(error.str());
	}

	add_map_emitter_impl(source);
}


void vchannel_emitter::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


bool vchannel_emitter::peek()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame() on vchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_impl();
}


bool vchannel_emitter::peek(vchannel_frame_params & params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_frame(params) on vchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_impl(params);
}


void vchannel_emitter::pop(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use pop_frame() on vchannel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	assert(tfdf_size() >= tfdf_buffer_size);
	pop_impl(tfdf_buffer);
}


void vchannel_emitter::set_frame_seq_no(uint64_t value)
{
	_frame_seq_no.value(value, _frame_seq_no.value_size());
}


frame_seq_no_t vchannel_emitter::get_frame_seq_no() const
{
	return _frame_seq_no;
}


void vchannel_emitter::increase_frame_seq_no()
{
	_frame_seq_no++;
}


uint16_t vchannel_emitter::frame_size_overhead() const
{
	uint16_t retval = 0;
	retval += detail::tf_header_t::extended_size_forecast(frame_seq_no_len());

	return retval;
}


void vchannel_emitter::emit_event(const emitter_event & evt)
{
	_event_callback(evt);
}


void vchannel_emitter::finalize_impl()
{
	const auto min_frame_size = frame_size_overhead();
	if (tfdf_size() < min_frame_size)
	{
		std::stringstream error;
		error << "invalid frame size l2 on vchannel: " << tfdf_size() << ". "
				<< "It should be no less than " << min_frame_size;
		throw einval_exception(error.str());
	}
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


vchannel_acceptor::vchannel_acceptor(gvcid_t gvcid_)
	: channel_id(gvcid_)
{
	set_event_callback(&_default_event_callback<acceptor_event>);
}


void vchannel_acceptor::set_event_callback(event_callback_t event_callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callbck() on vchannel sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(event_callback);
}


void vchannel_acceptor::add_map_accceptor(map_acceptor * sink)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use add_map_sink() on vchannel sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	if (sink->channel_id.gvcid() != this->channel_id)
	{
		std::stringstream error;
		error << "invalid gmap id "
				<< static_cast<int>(sink->channel_id.sc_id())
				<< "," << static_cast<int>(sink->channel_id.vchannel_id())
				<< "," << static_cast<int>(sink->channel_id.map_id())
				<< " for map channel attached to vchannel sink"
				<< static_cast<int>(this->channel_id.sc_id())
				<< "," << static_cast<int>(this->channel_id.vchannel_id())
		;

		throw einval_exception(error.str());
	}


	add_map_acceptor_impl(sink);
}


void vchannel_acceptor::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


void vchannel_acceptor::finalize_impl()
{
	// Ничего пока не делаем. Чую тут будут танцы с фармом
}


void vchannel_acceptor::push(
		const vchannel_frame_params & frame_params,
		const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use push() on vchannel sink, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	push_impl(frame_params, tfdf_buffer, tfdf_buffer_size);
}


void vchannel_acceptor::emit_event(const acceptor_event & evt)
{
	_event_callback(evt);
}


}}
