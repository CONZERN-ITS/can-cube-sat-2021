#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_

#include <map>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>

namespace ccsds { namespace uslp {


class mchannel_rr_multiplexor: public pchannel_service
{
public:
	mchannel_rr_multiplexor(std::string name);
	virtual ~mchannel_rr_multiplexor() = default;

	virtual void frame_size(int32_t value) override;

	virtual bool peek_frame() override;
	virtual bool peek_frame(pchannel_frame_params_t & frame_params) override;
	virtual void pop_frame(uint8_t * frame_buffer) override;
	virtual void pop_frame() override;

	virtual void push_frame(uint8_t * frame_buffer) override;

	void attach_master_channel_service(mchannel_service * mchannel);

protected:
	typedef std::map<mcid_t, mchannel_service*> container_t;
	mchannel_service * _select_next_to_pop();
	container_t::iterator _rr_iterator;

private:
	mchannel_service * _selected_mchannel = nullptr;
	container_t _mchannels;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_MASTER_CHANNEL_MULTIPLEXOR_HPP_ */
