#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>

#include <assert.h>

namespace ccsds { namespace uslp {


vchannel_rr_muxer::vchannel_rr_muxer(mcid_t mcid_)
		: mchannel_emitter(mcid_)
{

}


void vchannel_rr_muxer::add_vchannel_source_impl(vchannel_emitter * source)
{
	_muxer.add_source(source);
}


void vchannel_rr_muxer::finalize_impl()
{
	mchannel_emitter::finalize_impl();

	const auto upper_size = this->tfdf_size();

	// Идем в двах прохода, чтобы в случае ошибки объекты были в более менее понятном состоянии
	for (auto * vchannel: _muxer)
		vchannel->tfdf_size(upper_size);

	for (auto * vchannel: _muxer)
		vchannel->finalize();
}


bool vchannel_rr_muxer::peek_frame_du_impl()
{
	if (!_selected_vchannel)
		_selected_vchannel = _muxer.select_next();

	if (!_selected_vchannel)
		return false;

	return _selected_vchannel->peek();
}


bool vchannel_rr_muxer::peek_frame_du_impl(mchannel_frame_params_t & frame_params)
{
	if (!_selected_vchannel)
		_selected_vchannel = _muxer.select_next();

	if (!_selected_vchannel)
		return false;

	vchannel_frame_params vparams;
	bool retval = _selected_vchannel->peek(vparams);
	if (!retval)
		return false;

	frame_params.channel_id = vparams.channel_id;
	frame_params.id_is_destination = this->id_is_destination();
	frame_params.frame_class = vparams.frame_class;
	frame_params.ocf_present = _ocf_source != nullptr;
	frame_params.frame_seq_no = vparams.frame_seq_no;
	frame_params.payload_cookies = std::move(vparams.payload_cookies);

	return true;
}


void vchannel_rr_muxer::pop_frame_du_impl(uint8_t * frame_data_field)
{
	assert(_selected_vchannel);

	auto * selected_vchannel_copy = _selected_vchannel;
	_selected_vchannel = nullptr;
	selected_vchannel_copy->pop(frame_data_field, tfdf_size());
}


uint16_t vchannel_rr_muxer::tfdf_size() const
{
	return frame_du_size_l1() - frame_size_overhead();
}


}}
