#include <ccsds/sdl/tc/mchannel/vchannel_rr_multiplexor.hpp>
#include <cstring>
#include <cassert>

#include <endian.h>


namespace ccsds { namespace sdl { namespace tc {


	vchannel_rr_multiplexor::vchannel_rr_multiplexor(
			mcid_t mcid, vfame_provider * provider
	) : mchannel_provider(mcid), _vprovider(provider)
	{

	}


	bool vchannel_rr_multiplexor::peek_frame(frame_params & fparams)
	{
		payload_params pparams;
		bool peek_result = _vprovider->peek_frame_payload(pparams);
		if (!peek_result)
			return false;

		_calc_frame_params(pparams, fparams);
		return true;
	}


	void vchannel_rr_multiplexor::pop_frame(uint8_t * frame_buffer)
	{
		frame_params fparams;
		bool peek_result = peek_frame(fparams);
		assert(peek_result);

		// Собираем заголовок
		frame_header hdr;
		hdr.gvcid(fparams.gvcid);
		hdr.frame_type(fparams.frame_type);
		hdr.frame_size(fparams.frame_size);
		hdr.seq_no(fparams.seq_number);
		hdr.pack(frame_buffer);

		// выгружаем полезную нагрузку фрейма
		uint8_t * const payload_buffer = frame_buffer + frame_header_size8;
		_vprovider->pop_frame_payload(payload_buffer);

		// Считаем контрольную сумму
		if (_use_crc)
		{
			const size_t frame_size_wo_crc = fparams.frame_size - frame_crc_size8;

			uint8_t * crc_addr = frame_buffer + frame_size_wo_crc;
			uint16_t crc_value = _calc_checksum(frame_buffer, frame_size_wo_crc);
			std::memcpy(crc_addr, &crc_value, sizeof(crc_value));
		}
	}


	void vchannel_rr_multiplexor::pop_frame()
	{
		_vprovider->pop_frame_payload();
	}


	void vchannel_rr_multiplexor::_calc_frame_params(
			const payload_params & pparams, frame_params & fparams
	) const
	{
		fparams.frame_type = pparams.frame_type;
		fparams.frame_size = pparams.payload_size + frame_header_size8;
		if (_use_crc)
			fparams.frame_size += frame_crc_size8;

		fparams.gvcid = gvcid_t(this->channel_id, pparams.vchannel_id);
		fparams.seq_number = pparams.seq_number;
	}


	uint16_t vchannel_rr_multiplexor::_calc_checksum(uint8_t * buffer, size_t buffer_size) const
	{
		// FIXME: Сделать нормальную контрольную сумму!
		return 0xDEAD;
	}


}}}
