#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_


#include <cstdint>
#include <vector>

#include <ccsds/uslp/common/defs.hpp>


namespace ccsds { namespace uslp {


struct event
{
	enum class kind_t
	{
		MAP_SDU
	};

	event(kind_t kind_): kind(kind_) {}
	virtual ~event() = default;

	const kind_t kind;
};


struct map_sdu_event: public event
{
	enum data_flags_t
	{
		INCOMPLETE		= 0x0001,
		IDLE			= 0x0002,
		CORRUPTED		= 0x0004,
		MAPA			= 0x0008,
		MAPP			= 0x0010
	};

	map_sdu_event(): event(event::kind_t::MAP_SDU) {} // @suppress("Class members should be properly initialized")

	std::vector<uint8_t> data;
	qos_t qos;
	uint64_t flags = 0;
};


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_ */
