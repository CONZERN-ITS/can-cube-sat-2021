#include <ccsds/uslp/map/abstract_map.hpp>

#include <sstream>

#include <ccsds/uslp/exceptions.hpp>

namespace ccsds { namespace uslp {


map_source::map_source(gmap_id_t map_id_)
	: map_id(map_id_)
{

}


map_source::map_source(gmap_id_t map_id_, uint16_t tfdf_size_)
	: map_id(map_id_), _tfdf_size(tfdf_size_)
{

}


void map_source::tfdf_size(uint16_t value)
{
	if (_finalized)
	{
		std::stringstream error;
		error << "unable to use tfdf_size() on map channel, because it is finalized";
		throw object_is_finalized(error.str());
	}

	_tfdf_size = value;
}



void map_source::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


bool map_source::peek_tfdf()
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf() on map_channel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_tfdf_impl();
}


bool map_source::peek_tfdf(tfdf_params & params)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf(params) on map_channel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	return peek_tfdf_impl(params);
}


void map_source::pop_tfdf(uint8_t * tfdf_buffer)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use peek_tfdf() on map_channel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	pop_tfdf_impl(tfdf_buffer);
}


void map_source::finalize_impl()
{
	// Тут нам нечего проверять так то...
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


void map_sink::finalize()
{
	if (_finalized)
		return;

	finalize_impl();
	_finalized = true;
}


void map_sink::push(tfdf_params & params, const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size)
{
	if (!_finalized)
	{
		std::stringstream error;
		error << "unable to use push() on map_channel, because it is not finalized";
		throw object_is_finalized(error.str());
	}

	push_impl(params, tfdf_buffer, tfdf_buffer_size);
}


void map_sink::finalize_impl()
{
	// нечего!
}


}}
