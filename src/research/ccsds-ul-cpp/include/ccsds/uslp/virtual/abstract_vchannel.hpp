#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_

#include <cstdint>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>


namespace ccsds { namespace uslp {


class map_source;


struct vchannel_frame_params
{
	gmap_id_t channel_id;
	frame_class_t frame_class;
	uint64_t frame_seq_no;
	uint8_t frame_seq_no_length;
};


class vchannel_source
{
public:
	vchannel_source(gvcid_t gvcid_);
	virtual ~vchannel_source() = default;

	void frame_size_l2(uint16_t value);
	uint16_t frame_size_l2() const noexcept { return _frame_size_l2; }

	void frame_seq_no_len(uint16_t value);
	uint8_t frame_seq_no_len() const noexcept { return _frame_seq_no_len; }

	void add_map_source(map_source * source);

	void finalize();

	bool peek_frame();
	bool peek_frame(vchannel_frame_params & params);
	void pop_frame(uint8_t * tfdf_buffer);

	const gvcid_t gvcid;

protected:
	void set_frame_seq_no(uint64_t value);
	uint64_t get_frame_seq_no() const;
	void increase_frame_seq_no();
	uint16_t frame_size_overhead() const;

	virtual void add_map_source_impl(map_source * source) = 0;

	virtual void check_and_sync_config();

	virtual bool peek_frame_impl() = 0;
	virtual bool peek_frame_impl(vchannel_frame_params & params) = 0;
	virtual void pop_frame_impl(uint8_t * tfdf_buffer) = 0;

private:
	bool _finalized = false;
	uint64_t _frame_seq_no = 0;
	uint8_t _frame_seq_no_len = 0;
	uint16_t _frame_size_l2 = 0;
};


}}

#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_ */
