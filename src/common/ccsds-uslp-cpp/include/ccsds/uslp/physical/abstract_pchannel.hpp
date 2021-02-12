#ifndef INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_
#define INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_

#include <functional>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <ccsds/uslp/common/defs.hpp>
#include <ccsds/uslp/common/ids.hpp>
#include <ccsds/uslp/common/frame_seq_no.hpp>

#include <ccsds/uslp/events.hpp>


namespace ccsds { namespace uslp {


class mchannel_emitter;
class mchannel_acceptor;


struct pchannel_frame_params_t
{
	//! Идентифкатор канала
	gmapid_t channel_id;
	//! Значение флага id_is_destination
	bool id_is_destination;
	//! Класс фрейма
	frame_class_t frame_class;
	//! Есть ли в этом фрейме OCF
	bool ocf_present;
	//! Номер фрейма
	std::optional<frame_seq_no_t> frame_seq_no;
	//! Список кукисов юнитов полезной нагрузки, которые едут в этом фрейме
	std::vector<payload_cookie_t> payload_cookies;
};


class pchannel_emitter
{
public:
	typedef std::function<void(const emitter_event &)> event_callback_t;

	pchannel_emitter(std::string name_);
	virtual ~pchannel_emitter() = default;

	void frame_version_no(uint8_t value);
	uint8_t frame_version_no() const noexcept { return _frame_version_no; }

	void frame_size(size_t value);
	size_t frame_size() const noexcept { return _frame_size; }

	void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	void set_event_callback(event_callback_t event_callback);

	void add_mchannel_source(mchannel_emitter * source);

	void finalize();

	bool peek();
	bool peek(pchannel_frame_params_t & frame_params);
	void pop(uint8_t * frame_buffer, size_t frame_buffer_size);

	const std::string channel_id;

protected:
	uint16_t frame_size_overhead() const;
	void emit_event(const emitter_event  & evt);

	virtual void add_mchannel_source_impl(mchannel_emitter * source) = 0;

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


class pchannel_acceptor
{
public:
	typedef std::function<void(const acceptor_event &)> event_callback_t;

	pchannel_acceptor(std::string name_);
	virtual ~pchannel_acceptor() = default;

	void insert_zone_size(uint16_t value);
	uint16_t insert_zone_size() const noexcept { return _insert_zone_size; }

	void error_control_len(error_control_len_t value);
	error_control_len_t error_control_len() const noexcept { return _error_control_len; }

	void set_event_callback(event_callback_t event_callback);

	void add_mchannel_acceptor(mchannel_acceptor * sink);

	void finalize();

	void push_frame(const uint8_t * frame_buffer, size_t frame_buffer_size);

	const std::string channel_id;

protected:
	void emit_event(const acceptor_event & evt);

	virtual void finalize_impl();
	virtual void add_mchannel_acceptor_impl(mchannel_acceptor * sink) = 0;
	virtual void push_frame_impl(const uint8_t * frame_buffer, size_t frame_buffer_size) = 0;

private:
	event_callback_t _event_callback;
	bool _finalized = false;
	uint16_t _insert_zone_size = 0;
	error_control_len_t _error_control_len = error_control_len_t::ZERO;
};


}}



#endif /* INCLUDE_CCSDS_USLP_PHYSICAL_ABSTRACT_HPP_ */
