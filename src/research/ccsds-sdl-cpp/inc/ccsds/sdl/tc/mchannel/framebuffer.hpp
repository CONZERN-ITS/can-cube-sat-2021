#ifndef CCSDS_SDL_TC_MCHANNEL_FRAMEBUFFER_HPP_
#define CCSDS_SDL_TC_MCHANNEL_FRAMEBUFFER_HPP_

#include <memory>
#include <queue>
#include <cassert>
#include <cstring>

#include <ccsds/sdl/tc/primitives.hpp>
#include <ccsds/sdl/tc/mchannel/abstract.hpp>


namespace ccsds { namespace sdl { namespace tc {

	class mchannel_framebuffer: public mchannel_provider
	{
	public:
		mchannel_framebuffer() = default;
		mchannel_framebuffer(const mchannel_framebuffer & other) = delete;
		mchannel_framebuffer(mchannel_framebuffer && other) = default;
		mchannel_framebuffer & operator=(const mchannel_framebuffer & other) = delete;
		mchannel_framebuffer & operator=(mchannel_framebuffer && other) = default;
		mchannel_framebuffer(size_t queue_size_limit): _queue_size_limit(queue_size_limit) {}

		void queue_size_limit(size_t value) { _queue_size_limit = value; }
		size_t queue_size_limit() const { return _queue_size_limit; }


		virtual bool peek_frame(frame_params & frame_params) override
		{
			auto queue_size = _frames.size();
			if (!queue_size)
				return false;

			auto & front_record = _frames.front();
			// У нас нет параметров фрейма - придется их выкапывать
			frame_header hdr;
			hdr.unpack(front_record.data.get());

			frame_params.frame_type = hdr.frame_type();
			frame_params.seq_number = hdr.seq_no();
			frame_params.frame_size = hdr.frame_size();
			frame_params.gvcid = hdr.gvcid();
			return true;
		}


		virtual void pop_frame(uint8_t * frame_buffer) override
		{
			assert(!_frames.empty());

			auto & front_record = _frames.front();
			std::memcpy(frame_buffer, front_record.data.get(), front_record.size);
			_frames.pop();
		}


		virtual void pop_frame() override
		{
			assert(!_frames.empty());
			_frames.pop();
		}


	private:
		struct frame_record
		{
			std::unique_ptr<uint8_t[]> data;
			size_t size;
		};

		size_t _queue_size_limit = 0;
		std::queue<frame_record> _frames;
	};


}}}


#endif /* CCSDS_SDL_TC_MCHANNEL_FRAMEBUFFER_HPP_ */
