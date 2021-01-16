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
	mchannel_muxer_pchannel_source(std::string name_, int32_t frame_size_);
	mchannel_muxer_pchannel_source(std::string name_, int32_t frame_size_, error_control_len_t err_control_len_);
	virtual ~mchannel_muxer_pchannel_source() = default;

	virtual void frame_size(int32_t value) override;

	virtual bool peek_frame() override;
	virtual bool peek_frame(pchannel_frame_params_t & frame_params) override;
	virtual void pop_frame(uint8_t * frame_buffer) override;

	void attach_master_channel_source(mchannel_source * mchannel);

private:
	struct mchannel_status_checker_t
	{
		bool operator()(mchannel_source * source) const { return source->peek_frame(); }
	};

	typedef rr_muxer<mchannel_source*, mchannel_status_checker_t> _muxer_t;

	uint16_t _frame_data_unit_size();
	void _sync_data_unit_size();

	_muxer_t _muxer;
	mchannel_source * _selected_mchannel = nullptr;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_ */
