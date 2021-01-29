#include <ccsds/uslp/virtual/map_rr_muxer.hpp>
#include <cassert>


namespace ccsds { namespace uslp {



map_rr_muxer::map_rr_muxer(gvcid_t gvcid_)
	: vchannel_source(gvcid_)
{

}


void map_rr_muxer::add_map_source_impl(map_source * source)
{
	_muxer.add_source(source);
}


void map_rr_muxer::finalize_impl()
{
	vchannel_source::finalize_impl();

	// Проходим по мап каналам и ставим им размер tfdf
	const auto the_tfdf_size = tfdf_size() - frame_size_overhead();
	for (auto * map_source: _muxer)
		map_source->tfdf_size(the_tfdf_size);

	// Второй круг - вызываем финализацию
	for (auto * map_source: _muxer)
		map_source->finalize();
}


bool map_rr_muxer::peek_impl()
{
	if (!_selected_channel)
		_selected_channel = _muxer.select_next();

	if (!_selected_channel)
		return false;

	return _selected_channel->peek_tfdf();
}


bool map_rr_muxer::peek_impl(vchannel_frame_params & params)
{
	if (!_selected_channel)
		_selected_channel = _muxer.select_next();

	if (!_selected_channel)
		return false;

	output_map_frame_params map_params;
	bool retval = _selected_channel->peek_tfdf(map_params);
	if (!retval)
		return false;

	params.channel_id = _selected_channel->channel_id;

	switch (map_params.qos)
	{
	case qos_t::EXPIDITED:
		params.frame_class = frame_class_t::EXPEDITED_PAYLOAD;
		break;

	case qos_t::SEQUENCE_CONTROLLED:
		params.frame_class = frame_class_t::CONTROLLED_PAYLOAD;
		break;

	default:
		assert(false);
		break;
	};

	params.frame_seq_no = vchannel_source::get_frame_seq_no();
	return true;
}


void map_rr_muxer::pop_impl(uint8_t * tfdf_buffer)
{
	assert(_selected_channel);

	// Нам нужно посмотреть заблокириует ли этот map весь канал на себя или нет
	output_map_frame_params map_params;
	_selected_channel->peek_tfdf(map_params);

	auto * selected_channel_copy = _selected_channel;
	if (!map_params.channel_lock)
		_selected_channel = nullptr; // Если блокировки нет - снимаем этот канал с выбранного

	selected_channel_copy->pop_tfdf(tfdf_buffer, tfdf_size());
	vchannel_source::increase_frame_seq_no();
}


}}
