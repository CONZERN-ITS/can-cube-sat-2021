#ifndef INCLUDE_CCSDS_USLP_MAP_DETAIL_EPP_HEADER_HPP_
#define INCLUDE_CCSDS_USLP_MAP_DETAIL_EPP_HEADER_HPP_

#include <cstdint>
#include <optional>

namespace ccsds { namespace uslp { namespace detail {

//! Некоторые резервированные идентификаторы инкапсулируемых протоколов
/*! Идентификаторы назначаются SAN-ой. Тут только интересные нам */
enum class epp_protocol_id_t
{
	//! Encapsulation Idle Packet (the encapsulation data field, if present, contains no protocol data but only idle data)
	IDLE = 0x00,
	//! Internet Protocol Extension (IPE)
	IPE = 0x02,
	//! Extended Protocol ID for Encapsulation Packet Protocol
	EXTENDED = 0x06,
	//! Mission-specific, privately defined data
	PRIVATE = 0x07,
};

//! Заголовок EPP пакета
/*! Тут все очень сложно и заголовок бывает какой захочет длины (хоть весь пакет в 1 байт)
	Поэтому лучше почитать спеку */
struct epp_header
{
	//! packet version number для этих пакетов
	/*! Три бита по единичке. Живут в самом старшем разряде первого байта */
	static constexpr uint8_t pvn = 0x07;

	//! Проверка указанного буфера на то, что в нем лежит заголовок инкапсулейтед пакета
	/*! Вернет длину заголовка, или 0, если его там вовсе нет */
	static uint16_t probe_header_size(const uint8_t * buffer);

	//! Размер заголовка в байтах с теми значениями, что в нем есть сейчас
	uint16_t size() const;

	void write(uint8_t * buffer) const;
	void read(const uint8_t * buffer);

	//! Идентификатор протокола, который инкапсулирован в этом пакете \sa epp_protocol_id_t
	/*! Не более 3ех бит.
		Если это поле имеет значение - epp_protocol_id_t::EXTENDED - заголовок пакета будет не
		меньше чем 4 байта */
	uint8_t protocol_id;

	//! Некое поле для личного приминения пользователей.
	/*! Не более 4 бит. Если это поле определено - заголовок пакета будет не меньше чем 4 байта */
	std::optional<uint8_t> user_defined_field;

	//! Расширенный идентификатор протокола. Не более 4 бит.
	/*! SANA ведет отдельный каталог значений этого поля и оно имеет смысл (и обязательно к использованию)
		только если protocol_id == epp_protocol_id_t::EXTENDED.
		Если это поле определено - заголовок пакета будет не меньше чем 4 байта */
	std::optional<uint8_t> protocol_id_extension;

	//! Резервированное поле CCSDS - по конвенции заполняется нулями
	/*! Если это поле определено - заголовок пакета будет не меньше чем 8 байт */
	std::optional<uint16_t> ccsds_field;
	//! Длина пакета в байтах.
	/*! Мы тут ничего не выдумываем и пишем длину ровно в том виде, в каком ее пишут в канале:
		реальная длина - 1.
		Заголовок выравнивается исходя из значения */
	uint32_t packet_len;

protected:
	// Сборка второго байта заголовка
	uint8_t _make_second_byte() const;
	// "Вгрузка" второго байта заголовка
	void _load_second_byte(uint8_t byte2);

};

}}}


#endif /* INCLUDE_CCSDS_USLP_MAP_DETAIL_EPP_HEADER_HPP_ */
