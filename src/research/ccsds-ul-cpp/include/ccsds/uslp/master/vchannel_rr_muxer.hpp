#ifndef INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_
#define INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_


#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>


namespace ccsds { namespace uslp {


class vchannel_rr_muxer: public mchannel_source
{
public:
	vchannel_rr_muxer(mcid_t mcid_);
	virtual ~vchannel_rr_muxer() = default;

protected:
	virtual void add_vchannel_source_impl(vchannel_source * source) override;
	virtual void finalize_impl() override;
	virtual bool peek_frame_du_impl() override;
	virtual bool peek_frame_du_impl(mchannel_frame_params_t & frame_params) override;
	virtual void pop_frame_du_impl(uint8_t * frame_data_field) override;

private:
	struct vchannel_status_checker_t
	{
		const bool operator()(vchannel_source * source) { return source->peek(); }
	};

	typedef rr_muxer<vchannel_source*, vchannel_status_checker_t> muxer_t;

	uint16_t tfdf_size() const;

	muxer_t _muxer;
	vchannel_source * _selected_vchannel = nullptr;
	ocf_source * _ocf_source = nullptr;
};

}}


#endif /* INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_MUXER_MCHANNEL_SOURCE_HPP_ */
