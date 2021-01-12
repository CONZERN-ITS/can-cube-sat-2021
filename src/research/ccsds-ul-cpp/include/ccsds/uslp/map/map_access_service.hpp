#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_

#include <deque>
#include <memory>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/map/abstract.hpp>


namespace ccsds { namespace uslp {


class map_access_service: public map_service
{
public:

	map_access_service(gmap_id_t map_id_);
	virtual ~map_access_service() = default;

	virtual void tfdf_size(uint16_t value) override;

	virtual bool peek_tfdf() override;
	virtual bool peek_tfdf(tfdf_params & params) override;
	virtual void pop_tfdf(uint8_t * tfdf_buffer) override;

protected:
	//! В таком виде будем хранить передаваемые нами чанки
	struct data_unit_t
	{
		//! Данные, которые нужно передать
		std::deque<uint8_t> data;
		//! Исходный размер этих данных (от деки могут быть откусаны куски)
		size_t data_original_size;
		//! Каким типом передачи будем гнать этот блок данных
		qos_t qos;
	};

private:
	std::deque<data_unit_t> _chunked_deque;
};


}}



#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SERVICE_HPP_ */
