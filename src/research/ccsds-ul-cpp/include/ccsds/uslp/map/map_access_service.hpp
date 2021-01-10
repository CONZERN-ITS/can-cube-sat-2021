#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_

#include <queue>
#include <memory>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/map/abstract.hpp>
#include <ccsds/uslp/map/detail/chunked_queue_adapter.hpp>


namespace ccsds { namespace uslp {


class map_access_service: public map_service
{
public:

	map_access_service(gmap_id_t map_id_);
	virtual ~map_access_service() = default;

	virtual bool peek_tfdf() override;
	virtual bool peek_tfdf(tfdf_params & params) override;
	virtual void pop_tfdf(uint8_t * tfdf_buffer) override;

protected:
	//! В таком виде будем хранить передаваемые нами чанки
	struct data_unit_t
	{
		//! Данные, которые нужно передать
		std::unique_ptr<const uint8_t[]> data;
		//! Размер этих данных
		size_t data_size;
		//! Каким типом передачи будем гнать этот блок данных
		qos_t qos;

		//! Начало данных блока (для detail::chunked_queue_adadpter)
		const uint8_t * begin() const { return data.get(); }
		//! Конец данных блока (для detail::chunked_queue_adadpter)
		const uint8_t * end() const { return data.get() + data_size; }
	};

private:
	typedef std::queue<data_unit_t> data_queue_t;

	data_queue_t _data_queue;
	detail::chunked_queue_adadpter<data_queue_t> _chunk_reader;
};


}}



#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_ */
