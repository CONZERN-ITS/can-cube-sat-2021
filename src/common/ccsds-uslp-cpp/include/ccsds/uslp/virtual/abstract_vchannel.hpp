#ifndef INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_

#include <functional>
#include <cstdint>
#include <optional>
#include <vector>

#include <ccsds/uslp/events.hpp>
#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp {


class map_emitter;
class map_acceptor;


struct vchannel_frame_params
{
	gmapid_t channel_id;
	frame_class_t frame_class;
	std::optional<frame_seq_no_t> frame_seq_no;
	//! Список кукисов юнитов полезной нагрузки, которые едут в этом фрейме
	std::vector<payload_cookie_t> payload_cookies;
};


class vchannel_emitter
{
public:
	typedef std::function<void(const emitter_event &)> event_callback_t;

	vchannel_emitter(gvcid_t gvcid_);
	virtual ~vchannel_emitter() = default;

	void tfdf_size(uint16_t value);
	uint16_t tfdf_size() const noexcept { return _frame_size_l2; }

	void frame_seq_no_len(uint8_t value);
	uint8_t frame_seq_no_len() const noexcept { return _frame_seq_no.value_size(); }

	void set_event_callback(event_callback_t callback);

	void add_map_source(map_emitter * source);

	void finalize();

	bool peek();
	bool peek(vchannel_frame_params & params);
	void pop(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size);

	const gvcid_t channel_id;

protected:
	void set_frame_seq_no(uint64_t value);
	frame_seq_no_t get_frame_seq_no() const;
	void increase_frame_seq_no();
	uint16_t frame_size_overhead() const;

	virtual void add_map_emitter_impl(map_emitter * source) = 0;

	virtual void finalize_impl();

	virtual bool peek_impl() = 0;
	virtual bool peek_impl(vchannel_frame_params & params) = 0;
	virtual void pop_impl(uint8_t * tfdf_buffer) = 0;

	void emit_event(const emitter_event & evt);

private:
	event_callback_t _event_callback;
	bool _finalized = false;
	frame_seq_no_t _frame_seq_no;
	uint16_t _frame_size_l2 = 0;
};


class vchannel_acceptor
{
public:
	typedef std::function<void(const acceptor_event &)> event_callback_t;

	vchannel_acceptor(gvcid_t gvcid_);
	virtual ~vchannel_acceptor() = default;

	void set_event_callback(event_callback_t callback);

	void add_map_accceptor(map_acceptor * acceptor);

	void finalize();

	void push(
			const vchannel_frame_params & frame_params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	);

	const gvcid_t channel_id;

protected:
	void emit_event(const acceptor_event & evt);

	virtual void add_map_acceptor_impl(map_acceptor * acceptor) = 0;

	virtual void finalize_impl();

	virtual void push_impl(
			const vchannel_frame_params & frame_params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) = 0;

private:
	event_callback_t _event_callback;
	bool _finalized = false;
};


}}

#endif /* INCLUDE_CCSDS_USLP_VIRTUAL_ABSTRACT_HPP_ */
