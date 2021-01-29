#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SINK_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SINK_HPP_

#include <optional>
#include <limits>
#include <deque>

#include <ccsds/epp/epp_header.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>


namespace ccsds { namespace uslp {

class map_packet_sink: public map_sink
{
public:
	map_packet_sink(gmapid_t channel_id);
	virtual ~map_packet_sink() = default;

	void max_packet_size(size_t value);
	size_t max_packet_size() const { return _max_packet_size; }

	void emit_idle_packets(bool value);
	bool emit_idle_packets() const { return _emit_idle_packets; }

protected:
	virtual void finalize_impl() override;
	virtual void push_impl(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) override;

private:
	typedef std::deque<uint8_t> accum_t;

	void _consume_bytes(const uint8_t * begin, const uint8_t * end);
	void _parse_packets();
	void _flush_accum(accum_t::const_iterator flush_zone_end, int event_flags);
	void _flush_accum(int event_flags);
	void _drop_accum(accum_t::const_iterator drop_zone_end);

	//! QOS последнего пришедшего фрейма
	qos_t _prev_frame_qos;
	//! Номер последнего пришедшего фрейма
	std::optional<frame_seq_no_t> _prev_frame_seq_no;

	//! Буфер для накопления пакета
	accum_t _accumulator;
	//! Разобранный заголовок текущего пакета
	std::optional<epp::header_t> _current_packet_header;
	//! Максимально разрешенный размер пакета
	size_t _max_packet_size = std::numeric_limits<size_t>::max();
	//! Передавать ли idle пакеты в эвенты
	bool _emit_idle_packets = false;
};


}}



#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SINK_HPP_ */
