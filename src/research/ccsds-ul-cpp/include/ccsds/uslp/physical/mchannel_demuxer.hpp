#ifndef USLP_PHYSICAL_MCHANNEL_DEMUXER_HPP_
#define USLP_PHYSICAL_MCHANNEL_DEMUXER_HPP_


#include <map>

#include <ccsds/uslp/physical/abstract_pchannel.hpp>
#include <ccsds/uslp/master/abstract_mchannel.hpp>


namespace ccsds { namespace uslp {

	class mchannel_demuxer: public pchannel_sink
	{
	public:
		mchannel_demuxer(std::string name_);
		virtual ~mchannel_demuxer() = default;

	protected:
		typedef std::map<mcid_t, mchannel_sink *> container_t;

		virtual void finalize_impl() override;
		virtual void add_mchannel_sink_impl(mchannel_sink * sink) override;
		virtual void push_frame_impl(const uint8_t * frame_buffer, size_t frame_buffer_size) override;

	private:
		container_t _container;
	};
}}


#endif /* USLP_PHYSICAL_MCHANNEL_DEMUXER_HPP_ */
