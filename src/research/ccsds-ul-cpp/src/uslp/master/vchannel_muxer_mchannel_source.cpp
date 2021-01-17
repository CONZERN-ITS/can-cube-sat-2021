#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>

#include <assert.h>

namespace ccsds { namespace uslp {


vchannel_rr_muxer::vchannel_rr_muxer(mcid_t mcid_)
		: mchannel_source(mcid_)
{

}


void vchannel_rr_muxer::add_vchannel_source_impl(vchannel_source * source)
{
	_muxer.add_source(source);
}


void vchannel_rr_muxer::finalize_impl()
{
	mchannel_source::finalize_impl();

	// Выставляем размер уровнем выше как все что есть у нас без нашего оверхеда
	const auto upper_size = frame_size_l1() - frame_size_overhead();

	// Идем в двах прохода, чтобы в случае ошибки объекты были в более менее понятном состоянии
	for (auto * vchannel: _muxer)
		vchannel->frame_size_l2(upper_size);

	for (auto * vchannel: _muxer)
		vchannel->finalize();
}


bool vchannel_rr_muxer::peek_frame_impl()
{
	if (!_selected_vchannel)
		_selected_vchannel = _muxer.select_next();

	if (!_selected_vchannel)
		return false;

	return _selected_vchannel->peek_frame();
}


bool vchannel_rr_muxer::peek_frame_impl(mchannel_frame_params_t & frame_params)
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


void vchannel_rr_muxer::pop_frame_impl(uint8_t * frame_data_field)
{
	assert(_selected_vchannel);

	auto * selected_vchannel_copy = _selected_vchannel;
	_selected_vchannel = nullptr;
	selected_vchannel_copy->pop_frame(frame_data_field);
}


}}
