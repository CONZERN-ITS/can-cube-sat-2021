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


class mchannel_source;


struct pchannel_frame_params_t
{
	gmap_id_t channel_id;
	bool id_is_destination;
	frame_class_t frame_class;
	bool ocf_present;
	uint64_t frame_seq_no;
	uint8_t frame_seq_no_length;
};


class pchannel_source
{
public:
	pchannel_source(std::string name_);
	pchannel_source(std::string name_, int32_t frame_size_);
	pchannel_source(std::string name_, int32_t frame_size_, error_control_len_t err_control_len_);
	virtual ~pchannel_source() = default;

	virtual void frame_size(int32_t value);
	int32_t frame_size() const noexcept { return _frame_size; }

	virtual void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	virtual void add_mchannel_source(mchannel_source * source) = 0;

	virtual bool peek_frame() = 0;
	virtual bool peek_frame(pchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame(uint8_t * frame_buffer) = 0;

	const std::string name;

private:
	int32_t _frame_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_ */
