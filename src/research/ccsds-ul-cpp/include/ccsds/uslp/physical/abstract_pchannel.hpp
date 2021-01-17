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
	virtual ~pchannel_source() = default;

	void frame_size(int32_t value);
	int32_t frame_size() const noexcept { return _frame_size; }

	void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	void add_mchannel_source(mchannel_source * source);

	void finalize();

	bool peek_frame();
	bool peek_frame(pchannel_frame_params_t & frame_params);
	void pop_frame(uint8_t * frame_buffer);

	const std::string name;

protected:
	uint16_t frame_size_overhead() const;

	virtual void add_mchannel_source_impl(mchannel_source * source) = 0;

	virtual void check_and_sync_config();

	virtual bool peek_frame_impl() = 0;
	virtual bool peek_frame_impl(pchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame_impl(uint8_t * frame_buffer) = 0;

private:
	bool _finalized = false;
	int32_t _frame_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_ */
