#ifndef CCSDS_SDL_TC_PCHANNEL_ABSTRACT_HPP_
#define CCSDS_SDL_TC_PCHANNEL_ABSTRACT_HPP_


#include <ccsds/sdl/tc/mchannel/abstract.hpp>


namespace ccsds { namespace sdl { namespace tc {

	class pchannel_adapter
	{
	public:
		pchannel_adapter() = default;
		pchannel_adapter(size_t max_buffer_size_): _max_buffer_size(max_buffer_size_) {}

		virtual ~pchannel_adapter() = default;

		virtual bool peek_frame(frame_params & fparams) = 0;
		virtual void pop_frame(uint8_t * buffer) = 0;
		virtual void pop_frame() = 0;

		virtual void max_buffer_size(size_t value) { _max_buffer_size = value; }
		size_t max_buffer_size() const { return _max_buffer_size; }

	private:
		size_t _max_buffer_size = 0;
	};


}}}


#endif /* CCSDS_SDL_TC_PCHANNEL_ABSTRACT_HPP_ */
