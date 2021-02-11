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
		EMITTED_FRAME,
		ACCEPTED_MAP_SDU,
	};

protected:
	event(kind_t kind_): kind(kind_) {}

public:
	virtual ~event() = default;

	const kind_t kind;
};


struct emitter_event: public event
{
protected:
	emitter_event(kind_t kind_): event(kind_) {}
};


struct event_emitted_frame: public event
{
	event_emitted_frame(): event(kind_t::EMITTED_FRAME) {}  // @suppress("Class members should be properly initialized")

	payload_cookie_t payload_cookie;
};



struct acceptor_event: public event
{
protected:
	acceptor_event(kind_t kind_): event(kind_) {}
};


struct event_accepted_map_sdu: public acceptor_event
{
	enum data_flags_t
	{
		INCOMPLETE		= 0x0001,
		IDLE			= 0x0002,
		CORRUPTED		= 0x0004,
		MAPA			= 0x0008,
		MAPP			= 0x0010
	};

	event_accepted_map_sdu(): acceptor_event(kind_t::ACCEPTED_MAP_SDU) {} // @suppress("Class members should be properly initialized")

	std::vector<uint8_t> data;
	qos_t qos;
	uint64_t flags = 0;
};


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_STACK_EVENTS_HPP_ */
