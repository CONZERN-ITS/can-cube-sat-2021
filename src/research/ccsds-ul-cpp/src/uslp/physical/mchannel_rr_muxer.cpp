#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <cassert>

namespace ccsds { namespace uslp {


mchannel_rr_muxer::mchannel_rr_muxer(std::string name_)
	: pchannel_source(std::move(name_))
{

}


void mchannel_rr_muxer::add_mchannel_source_impl(mchannel_source * source)
{
	_muxer.add_source(source);
}


void mchannel_rr_muxer::check_and_sync_config()
{
	pchannel_source::check_and_sync_config();
	auto upper_size = _frame_size_l1();

	// Сначала выставляем настройки а потом финализируем
	// На случай исключений, чтобы все объекты были в понятном состоянии
	for (auto * mchannel: _muxer)
		mchannel->frame_size_l1(upper_size);


	for (auto * mchannel: _muxer)
		mchannel->finalize();
}


bool mchannel_rr_muxer::peek_frame_impl()
{
	if (!_selected_mchannel)
		_selected_mchannel = _muxer.select_next();

	if (!_selected_mchannel)
		return false;

	return _selected_mchannel->peek_frame();
}


bool mchannel_rr_muxer::peek_frame_impl(pchannel_frame_params_t & frame_params)
{
	if (!_selected_mchannel)
		_selected_mchannel = _muxer.select_next();

	if (!_selected_mchannel)
		return false;

	mchannel_frame_params_t mchannel_params;
	bool retval = _selected_mchannel->peek_frame(mchannel_params);
	if (!retval)
		return false;

	frame_params.channel_id = mchannel_params.channel_id;
	frame_params.id_is_destination = mchannel_params.id_is_destination;
	frame_params.frame_class = mchannel_params.frame_class;
	frame_params.ocf_present = mchannel_params.ocf_present;
	frame_params.frame_seq_no = mchannel_params.frame_seq_no;
	frame_params.frame_seq_no_length = mchannel_params.frame_seq_no_length;

	return true;
}


void mchannel_rr_muxer::pop_frame_impl(uint8_t * frame_buffer)
{
	assert(_selected_mchannel);

	auto selected_mchannel_copy = _selected_mchannel;
	_selected_mchannel = nullptr;

	selected_mchannel_copy->pop_frame(frame_buffer);
}


uint16_t mchannel_rr_muxer::_frame_size_l1()
{
	const auto frame_size = pchannel_source::frame_size();
	std::decay<decltype(frame_size)>::type retval = frame_size;

	// смотрим сколько байт мы отъедим на контрольную сумму
	switch (pchannel_source::error_control_len())
	{
	case error_control_len_t::ZERO: retval -= 0; break;
	case error_control_len_t::TWO_OCTETS: retval -= 2; break;
	case error_control_len_t::FOUR_OCTETS: retval -= 4; break;
	default:
		assert(false);
	};

	// TODO: сколько байт мы отъедим на insert сервис

	// Вот теперь собственно и все
	return static_cast<uint16_t>(retval);
}

}}



