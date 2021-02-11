#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>

#include <cassert>
#include <cstring>

#include <ccsds/uslp/_detail/tf_header.hpp>


namespace ccsds { namespace uslp {


mchannel_rr_muxer::mchannel_rr_muxer(std::string name_)
	: pchannel_emitter(std::move(name_))
{

}


void mchannel_rr_muxer::add_mchannel_source_impl(mchannel_emitter * source)
{
	_muxer.add_source(source);
}


void mchannel_rr_muxer::finalize_impl()
{
	pchannel_emitter::finalize_impl();
	auto upper_size = _frame_du_size();

	// Сначала выставляем настройки а потом финализируем
	// На случай исключений, чтобы все объекты были в понятном состоянии
	for (auto * mchannel: _muxer)
		mchannel->frame_du_size_l1(upper_size);


	for (auto * mchannel: _muxer)
		mchannel->finalize();
}


bool mchannel_rr_muxer::peek_impl()
{
	if (!_selected_mchannel)
		_selected_mchannel = _muxer.select_next();

	if (!_selected_mchannel)
		return false;

	return _selected_mchannel->peek_frame_du();
}


bool mchannel_rr_muxer::peek_impl(pchannel_frame_params_t & frame_params)
{
	if (!_selected_mchannel)
		_selected_mchannel = _muxer.select_next();

	if (!_selected_mchannel)
		return false;

	mchannel_frame_params_t mchannel_params;
	bool retval = _selected_mchannel->peek_frame_du(mchannel_params);
	if (!retval)
		return false;

	frame_params.channel_id = mchannel_params.channel_id;
	frame_params.id_is_destination = mchannel_params.id_is_destination;
	frame_params.frame_class = mchannel_params.frame_class;
	frame_params.ocf_present = mchannel_params.ocf_present;
	frame_params.frame_seq_no = mchannel_params.frame_seq_no;
	frame_params.payload_cookies = std::move(mchannel_params.payload_cookies);

	return true;
}


void mchannel_rr_muxer::pop_impl(uint8_t * frame_buffer)
{
	assert(_selected_mchannel);

	auto selected_mchannel_copy = _selected_mchannel;
	_selected_mchannel = nullptr;

	// Нам нужны параметры фрейма, чтобы слепить заголовок. Поэтому мы еще раз делаем peek_frame()
	pchannel_frame_params_t frame_params;
	peek(frame_params);

	// Теперь самая жесть.
	// Делаем заголовок
	detail::tf_header_t header;
	header.frame_version_no = frame_version_no();
	header.gmap_id = frame_params.channel_id;
	header.id_is_destination = frame_params.id_is_destination;

	// У нас всегда будет расширенный заголовок
	header.ext.emplace();
	header.ext->frame_seq_no = frame_params.frame_seq_no;
	header.ext->frame_class = frame_params.frame_class;
	header.ext->ocf_present = frame_params.ocf_present;
	// Тут несколько неочевидно
	header.ext->frame_len = frame_size() - 1; // Именно так следует писать длины по мнениею CCSDS

	// начинаем размечать фрейм
	// С заголовком все понятно
	// const size_t header_buffer_size = header.size();
	uint8_t * const header_buffer = frame_buffer;

	// Контрольная сумма (она может быть а может и нет
	const size_t error_check_buffer_size = frame_size_overhead();
	uint8_t * const error_check_buffer = (0 != error_check_buffer_size)
			? frame_buffer + frame_size() - static_cast<size_t>(error_control_len())
			: nullptr
	;

	// tfdf + ocf сразу за заголовком. Размер - весь фрейм - заголовок и контрольная сумма
	// так то может быть еще инсерт зона, но её пока нет
	// const size_t tfdf_and_ocf_buffer_size = frame_size() - header_buffer_size - error_check_buffer_size;
	uint8_t * const tfdf_and_ocf_buffer = header_buffer + header.size();

	// Разметили
	// пишем заголовок
	header.write(header_buffer);
	// Пишем фрейм
	selected_mchannel_copy->pop_frame_du(tfdf_and_ocf_buffer, _frame_du_size());
	// Пишем конетрольную сумму TODO!
	if (error_check_buffer)
		std::memset(error_check_buffer, 0, error_check_buffer_size);
}


uint16_t mchannel_rr_muxer::_frame_du_size() const
{
	const auto frame_size = pchannel_emitter::frame_size();
	std::decay<decltype(frame_size)>::type retval = frame_size;

	// смотрим сколько байт мы отъедим на контрольную сумму
	switch (pchannel_emitter::error_control_len())
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



