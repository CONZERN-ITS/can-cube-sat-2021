#ifndef INCLUDE_CCSDS_USLP_MAP_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_MAP_ABSTRACT_HPP_

#include <cstdint>

#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/ids.hpp>


namespace ccsds { namespace uslp {


enum class tfdz_construction_rule_t
{
	PACKET_STREAM = 0x00,
	MAP_ACCESS_SDU_START = 0x01,
	MAP_ACCESS_SDU_CONTINUE = 0x02,
};


//! Параметры пейлоада, отправляемого этим map каналом
struct tfdf_params
{
	//! тип гарантии доставки этого кадра
	qos_t qos;
	//! Можно ли переходить к следующему map каналу после отправки этого фрейма
	bool channel_lock;
};


class map_service
{
public:
	map_service(gmap_id_t map_id_): map_id(map_id_) {}
	virtual ~map_service() = default;

	virtual void tfdf_size(uint16_t value) { _tfdf_size = value; }
	uint16_t tfdf_size() const { return _tfdf_size; }

	virtual bool peek_tfdf() = 0;
	virtual bool peek_tfdf(tfdf_params & params) = 0;
	virtual void pop_tfdf(uint8_t * tfdf_buffer) = 0;

	const gmap_id_t map_id;

private:
	uint16_t _tfdf_size = 0;
};


}}




#endif /* INCLUDE_CCSDS_USLP_MAP_ABSTRACT_HPP_ */
