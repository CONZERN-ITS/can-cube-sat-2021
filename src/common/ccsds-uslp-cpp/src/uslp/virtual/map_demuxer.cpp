#include <ccsds/uslp/virtual/map_demuxer.hpp>

namespace ccsds { namespace uslp {

map_demuxer::map_demuxer(gvcid_t gvcid_)
	: vchannel_acceptor(gvcid_)
{

}


void map_demuxer::add_map_acceptor_impl(map_acceptor * acceptor)
{
	_container.insert(std::make_pair(acceptor->channel_id, acceptor));
}


void map_demuxer::finalize_impl()
{
	vchannel_acceptor::finalize_impl();

	for (auto & pair: _container)
		pair.second->finalize();
}


void map_demuxer::push_impl(
		const vchannel_frame_params & frame_params,
		const uint8_t * tfdf_buffer, uint16_t tfdf_buffer_size
)
{
	// Ищем нужного абонента
	auto itt = _container.find(frame_params.channel_id);
	if (itt == _container.end())
		return; // Не нашлось...

	input_map_frame_params map_params;
	map_params.frame_seq_no = frame_params.frame_seq_no;
	switch (frame_params.frame_class)
	{
	case frame_class_t::CONTROLLED_PAYLOAD:
		map_params.qos = qos_t::SEQUENCE_CONTROLLED;
		break;

	case frame_class_t::EXPEDITED_PAYLOAD:
		map_params.qos = qos_t::EXPIDITED;
		break;

	default:
		// таких вообще-то быть не должно
		return;
	}

	itt->second->push(map_params, tfdf_buffer, tfdf_buffer_size);
}


}}
