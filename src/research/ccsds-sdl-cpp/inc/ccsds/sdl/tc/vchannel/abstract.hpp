#ifndef CCSDS_SDL_TC_VCHANNEL_ABSTRACT_HPP_
#define CCSDS_SDL_TC_VCHANNEL_ABSTRACT_HPP_


#include <cstddef>
#include <cstdint>

#include <ccsds/ids.hpp>
#include <ccsds/sdl/tc/primitives.hpp>


namespace ccsds { namespace sdl { namespace tc {


	struct payload_params
	{
		frame_type_t frame_type;
		uint8_t seq_number;
		size_t payload_size;
		uint8_t vchannel_id;
	};


	class vfame_provider
	{
	public:
		vfame_provider() = default;
		virtual ~vfame_provider() = default;

		virtual bool peek_frame_payload(payload_params & params) = 0;
		virtual void pop_frame_payload(uint8_t * payload_buffer) = 0;
		virtual void pop_frame_payload() = 0;

		virtual void max_payload_size(size_t value) { _max_payload_size = value; }
		size_t max_payload_size() const { return _max_payload_size; }

	private:
		size_t _max_payload_size = 0;
	};


	class vchannel_provider: public vfame_provider
	{
	public:
		vchannel_provider(uint8_t vchannel_id_): vchannel_id(vchannel_id_) {}
		virtual ~vchannel_provider() {}

		const uint8_t vchannel_id;
	};

}}}

#endif /* CCSDS_SDL_TC_VCHANNEL_ABSTRACT_HPP_ */
