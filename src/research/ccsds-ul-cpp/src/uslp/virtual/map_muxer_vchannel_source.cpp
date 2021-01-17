#include <ccsds/uslp/virtual/map_muxer_vchannel_source.hpp>

#include <cassert>


namespace ccsds { namespace uslp {



map_muxer_vchannel_source::map_muxer_vchannel_source(gvcid_t gvcid_)
	: vchannel_source(gvcid_)
{

}


void map_muxer_vchannel_source::add_map_source_impl(map_source * source)
{
	_muxer.add_source(source);
}


void map_muxer_vchannel_source::check_and_sync_config()
{
	vchannel_source::check_and_sync_config();

	// Проходим по мап каналам и ставим им размер tfdf
	const auto tfdf_size = frame_size_l2() - frame_size_overhead();
	for (auto * map_source: _muxer)
		map_source->tfdf_size(tfdf_size);

	// Второй круг - вызываем финализацию
	for (auto * map_source: _muxer)
		map_source->finalize();
}


bool map_muxer_vchannel_source::peek_frame_impl()
{
	if (!_selected_channel)
		_selected_channel = _muxer.select_next();

	if (!_selected_channel)
		return false;

	return _selected_channel->peek_tfdf();
}


bool map_muxer_vchannel_source::peek_frame_impl(vchannel_frame_params & params)
{
	if (!_selected_channel)
		_selected_channel = _muxer.select_next();

	if (!_selected_channel)
		return false;

	tfdf_params tparams;
	bool retval = _selected_channel->peek_tfdf(tparams);
	if (!retval)
		return false;

	params.channel_id = _selected_channel->map_id;

	switch (tparams.qos)
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
	params.frame_seq_no_length = vchannel_source::frame_seq_no_len();
	return true;
}


void map_muxer_vchannel_source::pop_frame_impl(uint8_t * tfdf_buffer)
{
	assert(_selected_channel);

	// Нам нужно посмотреть заблокириует ли этот map весь канал на себя или нет
	tfdf_params tparams;
	_selected_channel->peek_tfdf(tparams);

	auto * selected_channel_copy = _selected_channel;
	if (!tparams.channel_lock)
		_selected_channel = nullptr; // Если блокировки нет - снимаем этот канал с выбранного

	selected_channel_copy->pop_tfdf(tfdf_buffer);
	vchannel_source::increase_frame_seq_no();
}


}}
