#ifndef INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <functional>

#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/operation_control_field.hpp>


namespace ccsds { namespace uslp {


class mc_ocf_service
{
public:
	mc_ocf_service(gvcid_t & vcid_): vcid(vcid_) {}
	virtual ~mc_ocf_service() = default;

	virtual bool peek_down_ocf_field() = 0;
	virtual void pop_down_ocf_field(operation_control_field & object) = 0;
	virtual void pop_down_ocf_field() = 0;
	virtual void push_up_ocf_field(const operation_control_field & object) = 0;

	const gvcid_t vcid;

private:

};


struct mchannel_frame_params_t
{
	gmap_id_t channel_id;
	std::optional<operation_control_field> ocf_field;
	frame_class_t frame_class;
	uint64_t frame_seq_no;
	uint8_t frame_seq_length;
};


class mchannel_service
{
public:
	mchannel_service(mcid_t mcid_): mcid(mcid_) {}
	virtual ~mchannel_service() = default;

	virtual void max_frame_size(uint16_t value);
	uint16_t max_frame_size() const { return _max_frame_size; }

	virtual bool peek_frame() = 0;
	virtual bool peek_frame(mchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame(uint8_t * frame_data_field) = 0;
	virtual void pop_frame() = 0;
	virtual void push_frame(mchannel_frame_params_t & frame_params, uint8_t * frame_buffer) = 0;

	const mcid_t mcid;

private:
	uint16_t _max_frame_size = 0;
};



class mchannel_with_mc_ocf_service: public mchannel_service
{
public:
	mchannel_with_mc_ocf_service(mcid_t mcid_): mchannel_service(mcid_) {}
	virtual ~mchannel_with_mc_ocf_service() = default;

	virtual void add_mc_ocf_service(mc_ocf_service * service) = 0;
};


}}

#endif /* INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_ */
