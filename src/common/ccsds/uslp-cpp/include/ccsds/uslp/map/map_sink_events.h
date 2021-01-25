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
	//! Причиина по которой SDU был передан пользователю
	enum class release_reason_t
	{
		//! Он пришел целиком! все довольно очевидно
		SDU_COMPLETE,
		//! Случилось переполнение буфера для накопления SDU
		OVERFLOW,
		//! Мы потеряли какой-то фрейм (то есть получили не тот, что ожидали)
		/*! И отдаем все что смогли накопить */
		SDU_TERMINATED,
		//! Плохой заголовок пакета для пакетного мап сервиса
		BAD_PACKET_HEADER,
	};

	map_sink_event_data_unit(): map_sink_event(event_kind_t::DATA_UNIT) {} // @suppress("Class members should be properly initialized")

	std::vector<uint8_t> data;
	qos_t qos;
	release_reason_t release_reason;
};

}}


#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_SINK_EVENTS_H_ */
