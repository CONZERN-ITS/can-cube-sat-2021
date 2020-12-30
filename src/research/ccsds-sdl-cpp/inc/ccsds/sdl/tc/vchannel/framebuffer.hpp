#ifndef CCSDS_SDL_TC_VCHANNEL_FRAMEBUFFER_HPP_
#define CCSDS_SDL_TC_VCHANNEL_FRAMEBUFFER_HPP_

#include <tuple>
#include <queue>
#include <memory>


#include <ccsds/sdl/tc/primitives.hpp>
#include <ccsds/sdl/tc/vchannel/abstract.hpp>


namespace ccsds { namespace sdl { namespace tc {

	class vchannel_framebuffer: public vchannel_provider
	{
	public:
		vchannel_framebuffer() = default;
		virtual ~vchannel_framebuffer() = default;

		virtual bool peek_frame_payload(payload_params & params) override;
		virtual void pop_frame_payload(uint8_t * frame_payload_buffer, size_t frame_payload_buffer_size) override;
		virtual void pop_frame_payload() override;

		void push_frame_payload(frame_type_t ftype, const uint8_t * payload, size_t payload_size);

	private:
		void queue_size_limit(size_t value) { _queue_size_limit = value; }
		size_t queue_size_limit() const { return _queue_size_limit; }


	private:
		struct frame_record
		{
			frame_type_t frame_type;
			std::unique_ptr<uint8_t[]> data;
			size_t data_size;
		};

		size_t _queue_size_limit = 0;
		std::queue<frame_record> _frames;

	};

}}}


#endif /* CCSDS_SDL_TC_VCHANNEL_FRAMEBUFFER_HPP_ */
