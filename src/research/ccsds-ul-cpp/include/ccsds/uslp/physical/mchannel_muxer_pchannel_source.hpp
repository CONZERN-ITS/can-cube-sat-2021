#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_

#include <map>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>

namespace ccsds { namespace uslp {


class mchannel_muxer_pchannel_source: public pchannel_source
{
public:
	mchannel_muxer_pchannel_source(std::string name);
	virtual ~mchannel_muxer_pchannel_source() = default;

protected:
	virtual void add_mchannel_source_impl(mchannel_source * mchannel) override;

	virtual void check_and_sync_config() override;

	virtual bool peek_frame_impl() override;
	virtual bool peek_frame_impl(pchannel_frame_params_t & frame_params) override;
	virtual void pop_frame_impl(uint8_t * frame_buffer) override;

private:
	struct mchannel_status_checker_t
	{
		bool operator()(mchannel_source * source) const { return source->peek_frame(); }
	};

	typedef rr_muxer<mchannel_source*, mchannel_status_checker_t> muxer_t;

	uint16_t _frame_size_l1();

	muxer_t _muxer;
	mchannel_source * _selected_mchannel = nullptr;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_ */