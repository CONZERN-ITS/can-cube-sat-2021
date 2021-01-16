#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_MAP_MUXER_VCHANNEL_SOURCE_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_MAP_MUXER_VCHANNEL_SOURCE_HPP_

#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>


namespace ccsds { namespace uslp {


class map_muxer_vchannel_source: public vchannel_source
{
public:
	map_muxer_vchannel_source(gvcid_t gvcid_);
	map_muxer_vchannel_source(gvcid_t gvcid_, uint16_t frame_size_l2_);

	virtual void frame_size_l2(uint16_t value) override;
	virtual void frame_seq_no_len(uint16_t value) override;

	virtual void add_map_source(map_source * source) override;

	virtual bool peek_frame() override;
	virtual bool peek_frame(vchannel_frame_params & params) override;
	virtual void pop_frame(uint8_t * tfdf_buffer) override;

private:
	struct map_status_checker_t
	{
		bool operator()(map_source * source) const {return source->peek_tfdf(); }
	};

	typedef rr_muxer<map_source*, map_status_checker_t> muxer_t;

	void _sync_tfdf_size();

	muxer_t _muxer;
	map_source * _selected_channel = nullptr;
};

}}


#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_MAP_MUXER_VCHANNEL_SOURCE_HPP_ */
