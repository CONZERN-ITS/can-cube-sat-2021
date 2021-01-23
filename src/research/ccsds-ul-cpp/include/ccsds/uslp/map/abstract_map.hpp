#ifndef INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_
#define INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_

#include <cstdint>
#include <functional>

#include <ccsds/uslp/defs.hpp>
#include <ccsds/uslp/ids.hpp>

#include <ccsds/uslp/map/map_sink_events.h>
#include <ccsds/uslp/common/frame_seq_no.hpp>


namespace ccsds { namespace uslp {


//! Параметры пейлоада, отправляемого этим map каналом
struct output_map_frame_params
{
	//! тип гарантии доставки этого кадра
	qos_t qos;
	//! Можно ли переходить к следующему map каналу после отправки этого фрейма
	bool channel_lock;
};


class map_source
{
public:
	map_source(gmap_id_t map_id_);
	map_source(gmap_id_t map_id_, uint16_t tfdf_size_);
	virtual ~map_source() = default;

	void tfdf_size(uint16_t value);
	uint16_t tfdf_size() const noexcept { return _tfdf_size; }

	void finalize();

	bool peek_tfdf();
	bool peek_tfdf(output_map_frame_params & params);
	void pop_tfdf(uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size);

	const gmap_id_t map_id;

protected:

	virtual void finalize_impl();

	virtual bool peek_tfdf_impl() = 0;
	virtual bool peek_tfdf_impl(output_map_frame_params & params) = 0;
	virtual void pop_tfdf_impl(uint8_t * tfdf_buffer) = 0;

private:
	bool _finalized = false;
	uint16_t _tfdf_size = 0;
};


//! Параметры входящего мап каналва
struct input_map_frame_params
{
	//! тип гарантии доставки этого кадра
	qos_t qos;
	//! Номер фрейма
	std::optional<frame_seq_no_t> frame_seq_no;
};


class map_sink
{
public:
	typedef std::function<void(const map_sink_event &)> event_callback_t;

	map_sink(gmap_id_t map_id_): map_id(map_id_) {}
	virtual ~map_sink() = default;

	void set_event_callback(event_callback_t event_callback);

	void finalize();

	void push(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	);

	const gmap_id_t map_id;

protected:
	void emit_event(const map_sink_event & event);
	virtual void finalize_impl();
	virtual void push_impl(
			const input_map_frame_params & params,
			const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
	) = 0;

	bool is_finalized() const { return _finalized; }

private:
	event_callback_t _event_callback;
	bool _finalized = false;
};

}}




#endif /* INCLUDE_CCSDS_USLP_MAP_ABSTRACT_MAP_HPP_ */
