#ifndef CCSDS_SDL_TC_MCHANNEL_VCHANNEL_RR_MULTIPLEXOR_HPP_
#define CCSDS_SDL_TC_MCHANNEL_VCHANNEL_RR_MULTIPLEXOR_HPP_


#include <vector>

#include <ccsds/sdl/tc/mchannel/abstract.hpp>
#include <ccsds/sdl/tc/vchannel/abstract.hpp>


namespace ccsds { namespace sdl { namespace tc {

	class vchannel_rr_multiplexor: public mchannel_provider
	{
	public:
		typedef std::vector<vchannel_provider *> container_t;
		vchannel_rr_multiplexor(mcid_t mcid, vfame_provider * provider);

		virtual bool peek_frame(frame_params & fparams) override;
		virtual void pop_frame(uint8_t * frame_buffer) override;
		virtual void pop_frame() override;

		void use_crc(bool value) { _use_crc = value; }
		bool use_crc() const { return _use_crc; }

	protected:
		void _calc_frame_params(const payload_params & pparams, frame_params & fparams) const;
		uint16_t _calc_checksum(uint8_t * buffer, size_t buffer_size) const;

	private:
		bool _use_crc = false;
		vfame_provider * _vprovider;
	};


}}}



#endif /* CCSDS_SDL_TC_MCHANNEL_VCHANNEL_RR_MULTIPLEXOR_HPP_ */
