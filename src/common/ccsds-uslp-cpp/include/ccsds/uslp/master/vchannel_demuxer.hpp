#ifndef INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_DEMUXER_HPP_
#define INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_DEMUXER_HPP_


#include <map>

#include <ccsds/uslp/master/abstract_mchannel.hpp>
#include <ccsds/uslp/virtual/abstract_vchannel.hpp>


namespace ccsds { namespace uslp {


class vchannel_demuxer: public mchannel_acceptor
{
public:
	vchannel_demuxer(mcid_t mcid_);
	virtual ~vchannel_demuxer() = default;

protected:
	typedef std::map<gvcid_t, vchannel_acceptor*> container_t;

	virtual void add_vchannel_acceptor_impl(vchannel_acceptor * sink) override;
	virtual void finalize_impl() override;
	virtual void push_impl(
			const mchannel_frame_params_t & frame_params,
			const uint8_t * frame_data_unit, uint16_t frame_data_unit_size
	) override;

private:
	container_t _container;
};


}}



#endif /* INCLUDE_CCSDS_USLP_MASTER_VCHANNEL_DEMUXER_HPP_ */
