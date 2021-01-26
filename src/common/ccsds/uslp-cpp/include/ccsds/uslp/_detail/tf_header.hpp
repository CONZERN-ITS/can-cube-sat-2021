#ifndef INCLUDE_CCSDS_USLP_PHYSICAL__DETAIL_TF_HEADER_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL__DETAIL_TF_HEADER_HPP_

#include <cstdint>
#include <optional>

#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>

#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp { namespace detail {


//! Кусок транспортного кадра, который может быть в заголовке, а может и не быть
struct tf_header_extended_part_t
{
	//! Минимальная длина расширенной части заголовка
	static constexpr uint16_t static_size = 3;
	//! Максимальная длина расширенной части заголовка
	static constexpr uint16_t maximum_size = 10;

	//! Длина расширенной части заголовка
	uint16_t size() const noexcept;

	//! Запись расширенного заголовка
	void write(uint8_t * buffer) const noexcept;
	//! Чтение расширенного заголовка
	void read(const uint8_t * buffer);

	//! Класс кадра
	frame_class_t frame_class;
	//! Длина кадра (не фактическая, а прям циферка из заголовка)
	uint16_t frame_len;
	//! Циклический номер фрейма
	std::optional<frame_seq_no_t> frame_seq_no;
	//! Флаг наличия OCF поля
	bool ocf_present;
};


//! Заголовок транспортного кадра
struct tf_header_t
{
	//! Минимально возможный размер заголовка
	/*! Он будет такой длины если состоит только из базовой части без опциональной вовсе */
	static constexpr uint16_t static_size = 4;

	//! Максимально возможный размер заголовка
	/*! Он будет такой длины, если состоит из базовой и расширенной части с максимальной длиной номера фрейма */
	static constexpr uint16_t maximum_size = static_size + tf_header_extended_part_t::maximum_size;

	//! Дефолтное значение для поля frame_version_no согласно USLP стандарту по которому это все сделано
	static constexpr uint8_t default_frame_version_no = 0x0C;

	//! Размер обрезанного заголовка
	static constexpr uint16_t truncated_size_forecast() noexcept { return static_size; }
	//! Размер расширенного заголовка с номером фрейма. Длина номера передается аргументом
	static uint16_t extended_size_forecast(uint8_t frame_seq_no_len);
	static uint16_t extended_size_forecast(std::optional<frame_seq_no_t> frame_seq_no)
	{
		return extended_size_forecast(frame_seq_no ? frame_seq_no->value_size() : 0);
	}

	//! Размер заголовка
	uint16_t size() const;
	//! Записать заголовок в буфер
	void write(uint8_t * buffer) const;
	//! Прочитать заголовок из буфера
	void read(const uint8_t * buffer);

	//! Номер версии фрейма
	uint8_t frame_version_no = default_frame_version_no;
	//! Идентификатор канала кадра
	gmapid_t gmap_id;
	//! Дополнение к идентификатору - показывает фрейм исходит с этого канала или предназначен для него
	bool id_is_destination;

	//! Расширенная часть заголовка
	std::optional<tf_header_extended_part_t> ext;
};


}}}

#endif /* INCLUDE_CCSDS_USLP_PHYSICAL__DETAIL_TF_HEADER_HPP_ */
