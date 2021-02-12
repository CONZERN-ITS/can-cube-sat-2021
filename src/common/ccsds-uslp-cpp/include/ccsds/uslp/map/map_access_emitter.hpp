#ifndef INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SOURCE_HPP_
#define INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SOURCE_HPP_


#include <deque>
#include <memory>

#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/map/abstract_map.hpp>


namespace ccsds { namespace uslp {


class map_access_emitter: public map_emitter
{
public:
	map_access_emitter(gmapid_t map_id_);
	virtual ~map_access_emitter() = default;

	void add_sdu(payload_cookie_t cookie, const uint8_t * data, size_t data_size, qos_t qos);

protected:
	virtual void finalize_impl() override;

	virtual bool peek_tfdf_impl() override;
	virtual bool peek_tfdf_impl(output_map_frame_params & params) override;
	virtual void pop_tfdf_impl(uint8_t * tfdf_buffer) override;

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
		//! Идентификатор данных
		payload_cookie_t cookie;
	};

	//! Размер зоны именно для полезной нагрузки (без заголовка)
	uint16_t _tfdz_size() const;

private:

	std::deque<data_unit_t> _data_queue;
};


}}



#endif /* INCLUDE_CCSDS_USLP_MAP_MAP_ACCESS_SOURCE_HPP_ */
