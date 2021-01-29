#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SOURCE_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SOURCE_HPP_


#include <deque>
#include <memory>

#include <ccsds/epp/epp_header.hpp>

#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>


namespace ccsds { namespace uslp {


class map_packet_source: public map_source
{
public:
	map_packet_source(gmapid_t map_id_);
	virtual ~map_packet_source() = default;

	void add_packet(const uint8_t * packet, size_t packet_size, qos_t qos);
	void add_encapsulate_data(const uint8_t * data, size_t data_size, qos_t qos, epp::protocol_id_t proto_id);

protected:
	virtual void finalize_impl() override;

	virtual bool peek_tfdf_impl() override;
	virtual bool peek_tfdf_impl(output_map_frame_params & params) override;
	virtual void pop_tfdf_impl(uint8_t * tfdf_buffer) override;

protected:
	//! В таком виде будем хранить передаваемые нами чанки
	struct data_unit_t
	{
		//! Пакет, которые нужно передать
		std::deque<uint8_t> packet;
		//! В процессе работы мы будем отпиливать кусочки пакета
		//! Поэтому тут мы сохраним его исходный размер - пригодится
		size_t original_packet_size;
		//! Каким типом передачи будем гнать этот пакет
		qos_t qos;
	};

	// Размер зоны именно для полезной нагрузки (без заголовка)
	uint16_t _tfdz_size() const;

	//! Записывает EPP IDLE пакет в указанный буфер указанного размера
	void _write_idle_packet(uint8_t * buffer, uint16_t idle_packet_size) const;

private:
	std::deque<data_unit_t> _data_queue;
};




}}




#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SOURCE_HPP_ */
