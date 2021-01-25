#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_

#include <cstdint>
#include <optional>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp {


class map_source;
class map_sink;


struct vchannel_frame_params
{
	gmap_id_t channel_id;
	frame_class_t frame_class;
	std::optional<frame_seq_no_t> frame_seq_no;
};


class vchannel_source
{
public:
	vchannel_source(gvcid_t gvcid_);
	virtual ~vchannel_source() = default;

	void tfdf_size(uint16_t value);
	uint16_t tfdf_size() const noexcept { return _frame_size_l2; }

	void frame_seq_no_len(uint8_t value);
	uint8_t frame_seq_no_len() const noexcept { return _frame_seq_no.value_size(); }

	void add_map_source(map_source * source);

	void finalize();

	bool peek();
	bool peek(vchannel_frame_params & params);
	void pop(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size);

	const gvcid_t gvcid;

protected:
	void set_frame_seq_no(uint64_t value);
	frame_seq_no_t get_frame_seq_no() const;
	void increase_frame_seq_no();
	uint16_t frame_size_overhead() const;

	virtual void add_map_source_impl(map_source * source) = 0;

	virtual void finalize_impl();

	virtual bool peek_impl() = 0;
	virtual bool peek_impl(vchannel_frame_params & params) = 0;
	virtual void pop_impl(uint8_t * tfdf_buffer) = 0;

private:
	bool _finalized = false;
	frame_seq_no_t _frame_seq_no;
	uint16_t _frame_size_l2 = 0;
};


class vchannel_sink
{
public:
	vchannel_sink(gvcid_t gvcid_);
	virtual ~vchannel_sink() = default;

	void add_map_sink(map_sink * sink);

	void finalize();

	void push(
			const vchannel_frame_params & frame_params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	);

	const gvcid_t gvcid;

protected:
	virtual void add_map_sink_impl(map_sink * sink) = 0;

	virtual void finalize_impl();

	virtual void push_impl(
			const vchannel_frame_params & frame_params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) = 0;

private:
	bool _finalized = false;
};


}}

#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_ */
