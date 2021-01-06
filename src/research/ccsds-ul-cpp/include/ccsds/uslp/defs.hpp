#ifndef INCLUDE_CCSDS_USLP_DEFS_HPP_
#define INCLUDE_CCSDS_USLP_DEFS_HPP_


namespace ccsds { namespace uslp {

	enum class frame_class_t
	{
		//! Фрейм класса AD. Содержит нагрузку и подлежит контролю FARM
		CONTROLLED_PAYLOAD = 0x00,
		//! Резервированное и не используемое значение
		RESERVED = 0x01,
		//! Фрейм класса BD. Содержит нагрузку и не подлежит контролю FARM
		EXPEDITED_PAYLOAD = 0x02,
		//! Фрейм класса BC. Содержит команду для FARM и не поделжит его контролю
		EXPEDITED_COMMAND = 0x03
	};


}}


#endif /* INCLUDE_CCSDS_USLP_DEFS_HPP_ */
