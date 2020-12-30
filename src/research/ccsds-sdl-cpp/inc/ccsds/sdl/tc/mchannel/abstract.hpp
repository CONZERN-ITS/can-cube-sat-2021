#ifndef CCSDS_SDL_TC_MCHANNEL_ABSTRACT_HPP_
#define CCSDS_SDL_TC_MCHANNEL_ABSTRACT_HPP_

#include <cstddef>
#include <cstdint>

#include <optional>

#include <ccsds/ids.hpp>
#include <ccsds/sdl/tc/primitives.hpp>


namespace ccsds { namespace sdl { namespace tc {


	struct frame_params
	{
		frame_type_t frame_type;
		uint8_t seq_number;
		size_t frame_size;
		gvcid_t gvcid;
	};


	class mframe_provider
	{
	public:
		mframe_provider() = default;
		mframe_provider(size_t max_frame_size): _max_frame_size(max_frame_size) {}
		virtual ~mframe_provider() = default;

		virtual bool peek_frame(frame_params & fparams) = 0;
		virtual void pop_frame(uint8_t * frame_buffer) = 0;
		virtual void pop_frame() = 0;

		virtual void max_frame_size(size_t value) { _max_frame_size = value; }
		size_t max_frame_size() const { return _max_frame_size; }

	private:
		size_t _max_frame_size = 0;
	};


	class mchannel_provider: public mframe_provider
	{
	public:
		mchannel_provider(mcid_t channel_id_): channel_id(channel_id_) {}
		virtual ~mchannel_provider() = default;

		const mcid_t channel_id;
	};


}}}


#endif /* CCSDS_SDL_TC_MCHANNEL_ABSTRACT_HPP_ */
