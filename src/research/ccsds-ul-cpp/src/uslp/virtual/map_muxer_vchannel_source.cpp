#include <ccsds/uslp/virtual/map_muxer_vchannel_source.hpp>

#include <cassert>


namespace ccsds { namespace uslp {



map_muxer_vchannel_source::map_muxer_vchannel_source(gvcid_t gvcid_)
	: vchannel_source(gvcid_)
{

}


map_muxer_vchannel_source::map_muxer_vchannel_source(gvcid_t gvcid_, uint16_t frame_size_l2_)
	: vchannel_source(gvcid_, frame_size_l2_)
{

}


void map_muxer_vchannel_source::frame_size_l2(uint16_t value)
{
	const auto old_value = vchannel_source::frame_size_l2();

	vchannel_source::frame_size_l2(value);

	try
	{
		_sync_tfdf_size();
	}
	catch (...)
	{
		vchannel_source::frame_size_l2(old_value);
		throw;
	}
}


void map_muxer_vchannel_source::frame_seq_no_len(uint16_t value)
{
	const auto old_value = vchannel_source::frame_seq_no_len();

	vchannel_source::frame_seq_no_len(value);

	try
	{
		_sync_tfdf_size();
	}
	catch (...)
	{
		vchannel_source::frame_seq_no_len(old_value);
		throw;
	}
}


void map_muxer_vchannel_source::add_map_source(map_source * source)
{
	source->tfdf_size(tfdf_size());
	_muxer.add_source(source);
}


bool map_muxer_vchannel_source::peek_frame()
{
	if (!_selected_channel)
		_selected_channel = _muxer.select_next();

	if (!_selected_channel)
		return false;

	return _selected_channel->peek_tfdf();
}


bool map_muxer_vchannel_source::peek_frame(vchannel_frame_params & params)
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


void map_muxer_vchannel_source::pop_frame(uint8_t * tfdf_buffer)
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


void map_muxer_vchannel_source::_sync_tfdf_size()
{
	const auto new_size = tfdf_size();
	for (auto * source: _muxer)
		source->tfdf_size(new_size);
}

}}
