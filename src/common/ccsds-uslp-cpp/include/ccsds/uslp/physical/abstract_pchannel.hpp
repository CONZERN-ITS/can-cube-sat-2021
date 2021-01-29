#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_

#include <functional>
#include <cstdint>
#include <cstddef>
#include <string>

#include <ccsds/uslp/events.hpp>
#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp {


class mchannel_source;
class mchannel_sink;


struct pchannel_frame_params_t
{
	gmapid_t channel_id;
	bool id_is_destination;
	frame_class_t frame_class;
	bool ocf_present;
	std::optional<frame_seq_no_t> frame_seq_no;
};


class pchannel_source
{
public:
	typedef std::function<void(const event &)> event_callback_t;

	pchannel_source(std::string name_);
	virtual ~pchannel_source() = default;

	void frame_version_no(uint8_t value);
	uint8_t frame_version_no() const noexcept { return _frame_version_no; }

	void frame_size(size_t value);
	size_t frame_size() const noexcept { return _frame_size; }

	void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	void set_event_callback(event_callback_t event_callback);

	void add_mchannel_source(mchannel_source * source);

	void finalize();

	bool peek();
	bool peek(pchannel_frame_params_t & frame_params);
	void pop(uint8_t * frame_buffer, size_t frame_buffer_size);

	const std::string channel_id;

protected:
	uint16_t frame_size_overhead() const;
	void emit_event(const event & evt);

	virtual void add_mchannel_source_impl(mchannel_source * source) = 0;

	virtual void finalize_impl();

	virtual bool peek_impl() = 0;
	virtual bool peek_impl(pchannel_frame_params_t & frame_params) = 0;
	virtual void pop_impl(uint8_t * frame_buffer) = 0;

private:
	event_callback_t _event_callback;
	uint8_t _frame_version_no;
	bool _finalized = false;
	size_t _frame_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
};


class pchannel_sink
{
public:
	typedef std::function<void(const event &)> event_callback_t;

	pchannel_sink(std::string name_);
	virtual ~pchannel_sink() = default;

	void insert_zone_size(uint16_t value);
	uint16_t insert_zone_size() const noexcept { return _insert_zone_size; }

	void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	void set_event_callback(event_callback_t event_callback);

	void add_mchannel_sink(mchannel_sink * sink);

	void finalize();

	void push_frame(const uint8_t * frame_buffer, size_t frame_buffer_size);

	const std::string channel_id;

protected:
	void emit_event(const event & evt);

	virtual void finalize_impl();
	virtual void add_mchannel_sink_impl(mchannel_sink * sink) = 0;
	virtual void push_frame_impl(const uint8_t * frame_buffer, size_t frame_buffer_size) = 0;

private:
	event_callback_t _event_callback;
	bool _finalized = false;
	uint16_t _insert_zone_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_ */
