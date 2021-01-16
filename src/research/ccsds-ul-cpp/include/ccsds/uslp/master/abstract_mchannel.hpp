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


class vchannel_source;


class ocf_source
{
public:
	virtual ~ocf_source() = default;

	virtual uint32_t get_ocf() = 0;
};


struct mchannel_frame_params_t
{
	gmap_id_t channel_id;
	bool id_is_destination;
	frame_class_t frame_class;
	bool ocf_present;
	uint64_t frame_seq_no;
	uint8_t frame_seq_no_length;
};


//! Абстрактный класс - источник транспортных фреймов уровня мастер канала
class mchannel_source
{
public:
	mchannel_source(mcid_t mcid_);
	mchannel_source(mcid_t mcid_, uint16_t frame_size_l1_);
	virtual ~mchannel_source() = default;

	virtual void frame_size_l1(uint16_t value);
	uint16_t frame_size_l1() const { return _frame_size_l1; }

	virtual void id_is_destination(bool value);
	bool id_is_destination() const noexcept { return _id_is_destination; }

	virtual void add_vchannel_source(vchannel_source * source) = 0;
	virtual void set_ocf_source(ocf_source * source) = 0;

	virtual bool peek_frame() = 0;
	virtual bool peek_frame(mchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame(uint8_t * frame_data_buffer) = 0;

	const mcid_t mcid;

private:
	bool _id_is_destination = true;
	uint16_t _frame_size_l1 = 0;
};



}}

#endif /* INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_ */
