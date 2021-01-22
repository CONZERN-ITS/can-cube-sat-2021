#ifndef INCLUDE_CCSDS_USLP_MAP_DETAIL_TFDF_HEADER_HPP_
#define INCLUDE_CCSDS_USLP_MAP_DETAIL_TFDF_HEADER_HPP_

#include <cstdint>
#include <optional>

namespace ccsds { namespace uslp { namespace detail {

//! указатель сегментирования данных map канала
struct tfdz_construction_rule_t
{
	tfdz_construction_rule_t() = delete;

	enum value {
		//! Стандартные пакеты размазанные по одному или нескольким транспортным фреймам
		PACKETS_SPANNING_MULTIPLE_FRAMES = 0x00,
		//! В этом фрейме начинается юнит MAP Access сервиса
		MAP_SDU_START = 0x01,
		//! В этом фрейме продолжается (или заканчивается) юнит MAP Access сервиса
		MAP_SDU_CONTINUATION = 0x02,
	};
};

//! Идентификатор типа данных в TFDZ
/*! Устаналивается SANA-ой. Тут приведены актуальные на данный момент и интересные для нас знчения */
struct upid_t
{
	upid_t() = delete;

	enum value {
		//! Space Packets or Encapsulation packets are contained within the TFDZ.
		CCSDS_PACKETS = 0x00,
		//! COP-1 Control Commands are contained within the TFDZ.
		COP_1_CTRL_COMMANDS = 0x01,
		//! COP-P Control Commands are contained within the TFDZ.
		COP_P_CTRL_COMMANDS = 0x02,
		//! Mission-specific information-1 as a MAPA_SDU is contained within the TFDZ.
		MAP_ACCESS_SDU = 0x05,
		//! UPID field signals that the entire fixed-length TFDZ contains idle data.
		IDLE = 0x1F
	};
};


//! Заголовок tfdz зоны
struct tfdf_header_t
{
	//! Длина короткого варианта заголовка
	static constexpr uint16_t short_size = 1;
	//! Длина полного варианта заголовка
	static constexpr uint16_t full_size = 3;
	//! Значение поля first_header_offset, которое означает что оффсета нет
	static constexpr uint16_t no_offset = 0xFFFF;

	//! Правило по которому она собиралась
	int ctr_rule;
	//! Идентификатор типа данных. которые содержаться в этом фрейме
	int upid;
	//! Оффсет до первого валидного заголовка пакета внутри фрейма
	/*! Или до последнего валидного байта юнита map access канала.
		Так-то это параметр опциональный, но для нашего сценария фиксированных по длине фреймов -
		не опциональный */
	std::optional<uint16_t> first_header_offset;

	//! Размер который такой заголовок займет в байтах
	uint16_t size() const { return first_header_offset ? full_size : short_size; }

	//! записать заголовок в буфер
	void write(uint8_t * buffer) const;
	//! Прочитать заголовок из буфера
	void read(const uint8_t * buffer, bool has_header_offset);

};


}}}

#endif /* INCLUDE_CCSDS_USLP_MAP_DETAIL_TFDF_HEADER_HPP_ */
