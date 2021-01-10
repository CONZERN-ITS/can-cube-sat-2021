#include <ccsds/uslp/physical/abstract_pchannel.hpp>

#include <sstream>

#include <ccsds/uslp/exceptions.hpp>
#include <ccsds/uslp/frame_reader.hpp>


namespace ccsds { namespace uslp {

void pchannel_service::frame_size(int32_t value)
{
	// TODO: Проверку на валидность значения
	_frame_size = value;
}


void pchannel_service::error_control_len(error_control_len_t value)
{
	_error_control_len = value;
}


void pchannel_service::register_insert_service(insert_zone_service * service)
{
	if (_insert_service)
	{
//		std::stringstream errstream;
//		errstream << "there is already insert channel service on physical channel " << name;
//		throw invalid_call_order(errstream.str());
	}

	_insert_service = service;
}


void pchannel_service::_write_insert_zone(uint8_t * zone_buffer)
{
	if (_insert_service)
		_insert_service->write_zone(zone_buffer);
}


void pchannel_service::_read_insert_zone(uint8_t * zone_buffer)
{
	if (_insert_service)
		_insert_service->read_zone(zone_buffer);
}


}}

