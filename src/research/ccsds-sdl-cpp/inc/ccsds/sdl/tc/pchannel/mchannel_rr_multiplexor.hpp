#ifndef CCSDS_SDL_TC_PCHANNEL_MCHANNEL_RR_MULTIPLEXOR_HPP_
#define CCSDS_SDL_TC_PCHANNEL_MCHANNEL_RR_MULTIPLEXOR_HPP_


#include <vector>

#include <ccsds/sdl/tc/pchannel/abstract.hpp>


namespace ccsds { namespace sdl { namespace tc {


	class mchannel_rr_multiplexor: public pchannel_adapter
	{
	public:
		typedef std::vector<mchannel_provider*> container_t;

		mchannel_rr_multiplexor() = default;
		virtual ~mchannel_rr_multiplexor() = default;

		void attach(mchannel_provider * provider);

		virtual bool peek_frame(frame_params & fparams) override;
		virtual void pop_frame(uint8_t * buffer) override;
		virtual void pop_frame() override;

		virtual void max_buffer_size(size_t value) override;

	protected:
		virtual mchannel_provider * _select_provider();

	private:
		container_t _providers;
		mchannel_provider * _selected_provider = nullptr;
		container_t::iterator _rr_iterator;
	};


}}}


#endif /* CCSDS_SDL_TC_PCHANNEL_MCHANNEL_RR_MULTIPLEXOR_HPP_ */
