#ifndef INCLUDE_CCSDS_USLP_DEFS_HPP_
#define INCLUDE_CCSDS_USLP_DEFS_HPP_


#include <cstdint>


namespace ccsds { namespace uslp {


enum class qos_t
{
	SEQUENCE_CONTROLLED,
	EXPIDITED,
};


//! Класс фрейма. Определяется двумя полями в заголовке
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


//! Тип поля контроля ошибок
enum class error_control_len_t
{
	//! Не присутствует
	ZERO = 0,
	//! Присутствует и имеет размер в 2 байта
	TWO_OCTETS = 2,
	//! Присутствует и имеет размер в 4 байта
	FOUR_OCTETS = 4,
};


}}


#endif /* INCLUDE_CCSDS_USLP_DEFS_HPP_ */
