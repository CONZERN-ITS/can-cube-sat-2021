#ifndef INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_
#define INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_


#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>


namespace ccsds { namespace uslp {


class vchannel_muxer_mchannel_source: public mchannel_source
{
public:
	vchannel_muxer_mchannel_source(mcid_t mcid_);
	virtual ~vchannel_muxer_mchannel_source() = default;

protected:
	virtual void add_vchannel_source_impl(vchannel_source * source) override;
	virtual void check_and_sync_config() override;
	virtual bool peek_frame_impl() override;
	virtual bool peek_frame_impl(mchannel_frame_params_t & frame_params) override;
	virtual void pop_frame_impl(uint8_t * frame_data_field) override;

private:
	struct vchannel_status_checker_t
	{
		bool operator()(vchannel_source * source) { return source->peek_frame(); }
	};

	typedef rr_muxer<vchannel_source*, vchannel_status_checker_t> muxer_t;

	muxer_t _muxer;
	vchannel_source * _selected_vchannel = nullptr;
	ocf_source * _ocf_source = nullptr;
};

}}


#endif /* INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_ */
