#ifndef INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_

#include <functional>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <ccsds/uslp/events.hpp>
#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp {


class vchannel_emitter;
class vchannel_acceptor;


class ocf_source
{
public:
	ocf_source() = default;
	virtual ~ocf_source() = default;

	virtual uint32_t get_ocf() = 0;
};


class ocf_sink
{
public:
	ocf_sink() = default;
	virtual ~ocf_sink() = default;

	virtual void put_ocf(uint32_t value) = 0;
};


struct mchannel_frame_params_t
{
	gmapid_t channel_id;
	bool id_is_destination;
	frame_class_t frame_class;
	bool ocf_present;
	std::optional<frame_seq_no_t> frame_seq_no;
	//! Список кукисов юнитов полезной нагрузки, которые едут в этом фрейме
	std::vector<payload_cookie_t> payload_cookies;
};


//! Абстрактный класс - источник транспортных фреймов уровня мастер канала
class mchannel_emitter
{
public:
	typedef std::function<void(const emitter_event &)> event_callback_t;

	mchannel_emitter(mcid_t mcid_);
	virtual ~mchannel_emitter() = default;

	void frame_du_size_l1(uint16_t value);
	uint16_t frame_du_size_l1() const { return _frame_size_l1; }

	void id_is_destination(bool value);
	bool id_is_destination() const noexcept { return _id_is_destination; }

	void set_event_callback(event_callback_t event_callback);

	void add_vchannel_source(vchannel_emitter * source);
	void set_ocf_source(ocf_source * source);
	void reset_ocf_source() { set_ocf_source(nullptr); }

	void finalize();

	bool peek_frame_du();
	bool peek_frame_du(mchannel_frame_params_t & frame_params);
	void pop_frame_du(uint8_t * frame_data_unit_buffer, uint16_t frame_data_unit_size);

	const mcid_t channel_id;

protected:
	uint16_t frame_size_overhead() const;

	void emit_event(const emitter_event & evt);

	virtual void add_vchannel_source_impl(vchannel_emitter * source) = 0;

	virtual void finalize_impl();

	virtual bool peek_frame_du_impl() = 0;
	virtual bool peek_frame_du_impl(mchannel_frame_params_t & frame_params) = 0;
	virtual void pop_frame_du_impl(uint8_t * frame_data_unit_buffer) = 0;

private:
	event_callback_t _event_callback;
	ocf_source * _ocf_source = nullptr;
	bool _finalized = false;
	bool _id_is_destination = true;
	uint16_t _frame_size_l1 = 0;
};


class mchannel_acceptor
{
public:
	typedef std::function<void(const acceptor_event &)> event_callback_t;

	mchannel_acceptor(mcid_t mcid_);
	virtual ~mchannel_acceptor() = default;

	void add_vchannel_acceptor(vchannel_acceptor * sink);
	void set_event_callback(event_callback_t event_callback);
	void set_ocf_sink(ocf_sink * sink);

	void finalize();

	void push(
			const mchannel_frame_params_t & frame_params,
			const uint8_t * frame_data_unit, uint16_t frame_data_unit_size
	);

	const mcid_t channel_id;

protected:
	void emit_event(const acceptor_event & evt);

	virtual void add_vchannel_acceptor_impl(vchannel_acceptor * sink) = 0;

	virtual void finalize_impl();

	virtual void push_impl(
			const mchannel_frame_params_t & frame_params,
			const uint8_t * frame_data_unit, uint16_t frame_data_unit_size
	) = 0;

private:
	event_callback_t _event_callback;
	bool _finalized = false;
	ocf_sink * _ocf_sink = nullptr;
};


}}

#endif /* INCLUDE_CCSDS_USLP_MASTER_ABSTRACT_HPP_ */
