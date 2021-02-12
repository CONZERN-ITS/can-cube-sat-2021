#include <ccsds/uslp/map/abstract_map.hpp>

#include <sstream>
#include <cassert>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {


template<typename T>
static void _default_event_callback(const T &)
{

}


map_emitter::map_emitter(gmapid_t map_id_)
	: channel_id(map_id_)
{
	set_event_callback(_default_event_callback<emitter_event>);
}


void map_emitter::tfdf_size(uint16_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use tfdf_size() on map source, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_tfdf_size = value;
}


void map_emitter::set_event_callback(event_callback_t event_callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callback() on map source, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(event_callback);
}


void map_emitter::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


bool map_emitter::peek_tfdf()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf() on map source, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_tfdf_impl();
}


bool map_emitter::peek_tfdf(output_map_frame_params & params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf(params) on map source, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_tfdf_impl(params);
}


void map_emitter::pop_tfdf(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf() on map source, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	assert(tfdf_buffer_size >= tfdf_size());

	pop_tfdf_impl(tfdf_buffer);
}


void map_emitter::finalize_impl()
{
	// Тут нам нечего проверять так то...
}


void map_emitter::emit_event(const emitter_event & event)
{
	_event_callback(event);
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


map_acceptor::map_acceptor(gmapid_t map_id_)
	: channel_id(map_id_)
{
	set_event_callback(_default_event_callback<acceptor_event>);
}


void map_acceptor::set_event_callback(event_callback_t event_callback)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use set_event_callback() on map sink, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_event_callback = std::move(event_callback);
}


void map_acceptor::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


void map_acceptor::push(
		const input_map_frame_params & params,
		const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use push() on map sink, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	push_impl(params, tfdf_buffer, tfdf_buffer_size);
}


void map_acceptor::emit_event(const acceptor_event & event)
{
	_event_callback(event);
}


void map_acceptor::finalize_impl()
{
	// нечего делать!
}


}}
