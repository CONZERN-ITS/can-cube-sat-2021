#include <ccsds/uslp/master/vchannel_muxer_mchannel_source.hpp>

#include <assert.h>

namespace ccsds { namespace uslp {


vchannel_muxer_mchannel_source::vchannel_muxer_mchannel_source(mcid_t mcid_)
		: mchannel_source(mcid_)
{

}


vchannel_muxer_mchannel_source::vchannel_muxer_mchannel_source(mcid_t mcid_, uint16_t frame_size_l1_)
	: mchannel_source(mcid_, frame_size_l1_)
{

}


void vchannel_muxer_mchannel_source::frame_size_l1(uint16_t value)
{
	const auto old_value = mchannel_source::frame_size_l1();

	mchannel_source::frame_size_l1(value);

	try
	{
		_sync_frame_size();
	}
	catch (...)
	{
		mchannel_source::frame_size_l1(old_value);
		throw;
	}
}


void vchannel_muxer_mchannel_source::add_vchannel_source(vchannel_source * source)
{
	source->frame_size_l2(_frame_size_l2());
	_muxer.add_source(source);
}


void vchannel_muxer_mchannel_source::set_ocf_source(ocf_source * source)
{
	bool sync_needed = _ocf_source == nullptr;
	_ocf_source = source;
	try
	{
		if (sync_needed)
			_sync_frame_size();
	}
	catch (...)
	{
		_ocf_source = nullptr;
		throw;
	}
}


bool vchannel_muxer_mchannel_source::peek_frame()
{
	if (!_selected_vchannel)
		_selected_vchannel = _muxer.select_next();

	if (!_selected_vchannel)
		return false;

	return _selected_vchannel->peek_frame();
}


bool vchannel_muxer_mchannel_source::peek_frame(mchannel_frame_params_t & frame_params)
{
	if (!_selected_vchannel)
		_selected_vchannel = _muxer.select_next();

	if (!_selected_vchannel)
		return false;

	vchannel_frame_params vparams;
	bool retval = _selected_vchannel->peek_frame(vparams);
	if (!retval)
		return false;

	frame_params.channel_id = vparams.channel_id;
	frame_params.id_is_destination = this->id_is_destination();
	frame_params.frame_class = vparams.frame_class;
	frame_params.ocf_present = _ocf_source != nullptr;
	frame_params.frame_seq_no = vparams.frame_seq_no;
	frame_params.frame_seq_no_length = vparams.frame_seq_no_length;

	return true;
}


void vchannel_muxer_mchannel_source::pop_frame(uint8_t * frame_data_field)
{
	assert(_selected_vchannel);

	auto * selected_vchannel_copy = _selected_vchannel;
	_selected_vchannel = nullptr;
	selected_vchannel_copy->pop_frame(frame_data_field);
}


uint16_t vchannel_muxer_mchannel_source::_frame_size_l2() const
{
	auto retval = this->mchannel_source::frame_size_l1();
	if (_ocf_source != 0)
		retval -= sizeof(uint32_t);

	return retval;
}


void vchannel_muxer_mchannel_source::_sync_frame_size()
{
	const auto size = _frame_size_l2();
	for (auto * source: _muxer)
		source->frame_size_l2(size);
}

}}
