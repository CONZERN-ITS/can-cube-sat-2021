#ifndef CCSDS_SDL_TC_PRIMITIVES_HPP_
#define CCSDS_SDL_TC_PRIMITIVES_HPP_

#include <ccsds/ids.hpp>

namespace ccsds { namespace sdl { namespace tc {


	//! Размер первичного заголовка транспортного кадра
	static constexpr size_t frame_header_size8 = 5;
	//! Размер контрольной суммы транспортного кадра
	static constexpr size_t frame_crc_size8 = 2;


	enum class frame_type_t
	{
		//! флаги заголовка bypass = 0, command = 0. Фрейм AB в терминах CCSDS
		FARM_CONTROLLED_PAYLOAD = 0x00,
		//! флаги заголовка bypass = 1, command = 1. Резервированный тип фрейма
		RESERVED = 0x01,
		//! Флаги заголовка bypass = 1, command = 0. Фрейм BD в терминах CCSDS
		FARM_BYPASS_PAYLOAD = 0x02,
		//! Флаги заголовка bypass = 0, command = 1. Фрейм BC в терминах CCSDS
		FARM_COMMAND = 0x03,
	};


	class frame_header
	{
	public:
		void gvcid(gvcid_t value) { _gvcid = value; }
		gvcid_t gvcid() const { return _gvcid; }

		void frame_size(size_t value) { _frame_len = value; }
		size_t frame_size() const { return _frame_len; }

		void frame_type(frame_type_t value) { _frame_type = value; }
		frame_type_t frame_type() const { return _frame_type; }

		void seq_no(uint8_t value) { _seq_no = value; }
		uint8_t seq_no() const { return _seq_no; }

		void pack(uint8_t * buffer);
		void unpack(uint8_t * buffer);

	private:
		gvcid_t _gvcid;
		size_t _frame_len;
		frame_type_t _frame_type;
		uint8_t _seq_no;
	};


}}}


#endif /* CCSDS_SDL_TC_PRIMITIVES_HPP_ */
