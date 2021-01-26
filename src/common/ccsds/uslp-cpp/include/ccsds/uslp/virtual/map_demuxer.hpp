#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_MAP_DEMUXER_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_MAP_DEMUXER_HPP_

#include <map>

#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>

namespace ccsds { namespace uslp {


class map_demuxer: public vchannel_sink
{
public:
	map_demuxer(gvcid_t gvcid_);
	virtual ~map_demuxer() = default;

protected:
	typedef std::map<gmapid_t, map_sink*> container_t;

	virtual void add_map_sink_impl(map_sink * sink) override;

	virtual void finalize_impl() override;

	virtual void push_impl(
			const vchannel_frame_params & frame_params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) override;

private:
	container_t _container;
};

}}


#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_MAP_DEMUXER_HPP_ */
