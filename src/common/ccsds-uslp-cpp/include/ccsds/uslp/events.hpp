#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_


#include <cstdint>
#include <vector>

#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>


namespace ccsds { namespace uslp {



struct emitter_event
{
	enum class kind_t
	{
		SDU_EMITTED,
	};

	emitter_event(kind_t kind_): kind(kind_) {}
	virtual ~emitter_event() = default;

	const kind_t kind;
};


struct emitter_event_sdu_emitted: public emitter_event
{
	emitter_event_sdu_emitted(): emitter_event(kind_t::SDU_EMITTED) {}
	virtual ~emitter_event_sdu_emitted() = default;

	payload_cookie_t payload_cookie = 0;
};



struct acceptor_event
{
	enum class kind_t
	{
		MAP_SDU,
	};


	acceptor_event(kind_t kind_): kind(kind_) {}
	virtual ~acceptor_event() = default;

	const kind_t kind;
};


struct acceptor_event_map_sdu: public acceptor_event
{
	enum data_flags_t
	{
		INCOMPLETE		= 0x0001,
		IDLE			= 0x0002,
		CORRUPTED		= 0x0004,
		MAPA			= 0x0008,
		MAPP			= 0x0010
	};

	acceptor_event_map_sdu(): acceptor_event(kind_t::MAP_SDU) {}

	gmapid_t channel_id;
	std::vector<uint8_t> data;
	qos_t qos = qos_t::EXPIDITED;
	uint64_t flags = 0;
};


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_ */
