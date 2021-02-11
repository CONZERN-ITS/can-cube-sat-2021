#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_

#include <map>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/common/rr_muxer.hpp>

namespace ccsds { namespace uslp {


class mchannel_rr_muxer: public pchannel_emitter
{
public:
	mchannel_rr_muxer(std::string name);
	virtual ~mchannel_rr_muxer() = default;

protected:
	virtual void add_mchannel_source_impl(mchannel_emitter * mchannel) override;

	virtual void finalize_impl() override;

	virtual bool peek_impl() override;
	virtual bool peek_impl(pchannel_frame_params_t & frame_params) override;
	virtual void pop_impl(uint8_t * frame_buffer) override;

private:
	struct mchannel_status_checker_t
	{
		const bool operator()(mchannel_emitter * source) const { return source->peek_frame_du(); }
	};

	typedef rr_muxer<mchannel_emitter*, mchannel_status_checker_t> muxer_t;

	uint16_t _frame_du_size() const;

	muxer_t _muxer;
	mchannel_emitter * _selected_mchannel = nullptr;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_ */
