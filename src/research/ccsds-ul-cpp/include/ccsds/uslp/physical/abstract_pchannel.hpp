#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_

#include <cstdint>
#include <cstddef>
#include <string>
#include <functional>

#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/operation_control_field.hpp>


namespace ccsds { namespace uslp {


class insert_zone_service
{
public:
	insert_zone_service(uint16_t zone_size_): zone_size(zone_size_) {}
	virtual ~insert_zone_service() = default;

	virtual void write_zone(uint8_t * zone_buffer) = 0;
	virtual void read_zone(const uint8_t * zone_buffer) = 0;

	const uint16_t zone_size;
};


struct pchannel_frame_params_t
{
	gmap_id_t map_id;
	uint16_t data_field_size;
	frame_class_t frame_class;
	std::optional<operation_control_field> ocf_field;
	uint64_t frame_seq_no;
	uint8_t frame_seq_length;
};


class pchannel_service
{
public:

	pchannel_service(std::string name_): name(name_) {}
	virtual ~pchannel_service() = default;

	virtual void frame_size(int32_t value);
	int32_t frame_size() const { return _frame_size; }

	virtual void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const { return _error_control_len; }

	void register_insert_service(insert_zone_service * service);
	uint16_t insert_zone_len() const { return _insert_service ? _insert_service->zone_size : 0; }

	virtual bool peek_frame() = 0;
	virtual bool peek_frame(pchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame(uint8_t * frame_buffer) = 0;
	virtual void pop_frame() = 0;

	virtual void push_frame(uint8_t * frame_buffer) = 0;

	const std::string name;

protected:
	void _write_insert_zone(uint8_t * zone_buffer);
	void _read_insert_zone(uint8_t * zone_buffer);

private:
	int32_t _frame_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
	insert_zone_service * _insert_service = nullptr;

};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_ */
