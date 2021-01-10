#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SERVICE_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SERVICE_HPP_



#include <deque>
#include <memory>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/map/abstract.hpp>
#include <ccsds/uslp/map/detail/chunked_queue_adapter.hpp>


namespace ccsds { namespace uslp {


class map_packet_service: public map_service
{
public:
	map_packet_service(gmap_id_t map_id_);
	virtual ~map_packet_service() = default;

	virtual void tfdf_size(uint16_t value) override;

	virtual bool peek_tfdf() override;
	virtual bool peek_tfdf(tfdf_params & params) override;
	virtual void pop_tfdf(uint8_t * tfdf_buffer) override;

protected:
	//! В таком виде будем хранить передаваемые нами чанки
	struct data_unit_t
	{
		//! Пакет, которые нужно передать
		std::unique_ptr<const uint8_t[]> packet;
		//! Размер этого пакета (со всеми его потрохами)
		size_t packet_size;
		//! Каким типом передачи будем гнать этот пакет
		qos_t qos;
		//! Нужно ли отправлять этот пакет прям сразу, даже если в фрейме после него будет свободное место
		bool flush_immidiately;

		//! Начало данных пакета (для detail::chunked_queue_adadpter)
		const uint8_t * begin() const { return packet.get(); }
		//! Конец данных пакета (для detail::chunked_queue_adadpter)
		const uint8_t * end() const { return packet.get() + packet_size; }
	};

	// Размер зоны именно для полезной нагрузки (без заголовка)
	uint16_t _tfdz_size() const;

	//! Будет ли заблокирован канал после выдачи следующего фрейма
	bool _will_lock_channel() const;

private:
	typedef std::deque<data_unit_t> data_queue_t;

	data_queue_t _data_queue;
	detail::chunked_queue_adadpter<data_queue_t> _chunk_reader;
};




}}




#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_PACKET_SERVICE_HPP_ */
