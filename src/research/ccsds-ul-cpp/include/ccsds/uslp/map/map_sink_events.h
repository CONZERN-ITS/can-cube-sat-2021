#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_SINK_EVENTS_H_
#define INCLUDE_CCSDS_USLP_MAP_MAP_SINK_EVENTS_H_


#include <cstdint>
#include <vector>

#include <ccsds/uslp/defs.hpp>


namespace ccsds { namespace uslp {


class map_sink_event
{
public:
	enum class event_kind_t
	{
		DATA_UNIT,
	};

	map_sink_event(event_kind_t kind_): kind(kind_) {}
	virtual ~map_sink_event() = default;

	const event_kind_t kind;
};


class map_sink_event_data_unit: public map_sink_event
{
public:
	map_sink_event_data_unit(std::vector<uint8_t> data_, qos_t qos_)
		: map_sink_event(event_kind_t::DATA_UNIT),
		  data(std::move(data_)), qos(qos_)
	{}

	std::vector<uint8_t> data;
	qos_t qos;
};

}}


#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_SINK_EVENTS_H_ */
