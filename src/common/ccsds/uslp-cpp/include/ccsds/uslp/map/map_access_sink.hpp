#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_


#include <functional>
#include <vector>
#include <limits>

#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>


namespace ccsds { namespace uslp {


namespace detail
{
	struct tfdf_header_t;
}


class map_access_sink: public map_sink
{
public:
	map_access_sink(gmapid_t mapid_);
	virtual ~map_access_sink() = default;

	void max_sdu_size(size_t value) { _max_sdu_size = value; }
	size_t max_sdu_size() const { return _max_sdu_size; }

protected:
	virtual void finalize_impl() override;

	virtual void push_impl(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) override;

private:
	void _consume_frame(
			const input_map_frame_params & params,
			const detail::tfdf_header_t & header,
			const uint8_t * tfdz, uint16_t tfdz_size
	);

	void _flush_accum(int event_flags);

	//! Аккумулятор для накопления полного SDU
	std::vector<uint8_t> _accumulator;

	//! Максимально допустимый размер SDU, который мы будем накапливать
	/*! Все что больше - будем дропать */
	size_t _max_sdu_size = std::numeric_limits<size_t>::max();

	//! QOS последнего пришедшего фрейма
	qos_t _prev_frame_qos;
	//! Номер последнего пришедшего фрейма
	std::optional<frame_seq_no_t> _prev_frame_seq_no;

	//! Коллбек для событий
	event_callback_t _event_callback;
};


}}


#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_ */
