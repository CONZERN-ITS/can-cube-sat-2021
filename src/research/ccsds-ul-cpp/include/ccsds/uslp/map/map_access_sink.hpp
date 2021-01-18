#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_


#include <functional>
#include <vector>

#include <ccsds/uslp/defs.hpp>
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
	map_access_sink(gmap_id_t mapid_);
	virtual ~map_access_sink() = default;

protected:
	virtual void finalize_impl() override;

	virtual void push_impl(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) override;

private:
	void consume_frame(
			const input_map_frame_params & params,
			const detail::tfdf_header_t & header,
			const uint8_t * tfdz, uint16_t tfdz_size
	);

	event_callback_t _event_callback;
	std::vector<uint8_t> _accumulator;
	qos_t _prev_frame_qos;
	frame_seq_no_t _prev_frame_seq_no;
};


}}


#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SINK_HPP_ */
