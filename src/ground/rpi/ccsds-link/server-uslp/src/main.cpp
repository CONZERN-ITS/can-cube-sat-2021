#include <zmq.hpp>

#include <ccsds/uslp/output_stack.hpp>
#include <ccsds/uslp/input_stack.hpp>

#include <ccsds/uslp/physical/mchannel_rr_muxer.hpp>
#include <ccsds/uslp/master/vchannel_rr_muxer.hpp>
#include <ccsds/uslp/virtual/map_rr_muxer.hpp>
#include <ccsds/uslp/map/map_packet_emitter.hpp>
#include <ccsds/uslp/map/map_access_emitter.hpp>

#include "json.hpp"


#define RADIO_FRAME_SIZE (200)


ccsds::uslp::input_stack istack;

class ostack
{
public:
	ostack()
	{
		using namespace ccsds;
		using namespace ccsds::uslp;

		auto phys = _stack.create_pchannel<mchannel_rr_muxer>("lora");
		phys->frame_size(RADIO_FRAME_SIZE);
		phys->error_control_len(error_control_len_t::ZERO);

		auto master = _stack.create_mchannel<vchannel_rr_muxer>(mcid_t(0x42));
		master->id_is_destination(true);

		auto virt = _stack.create_vchannel<map_rr_muxer>(gvcid_t(master->channel_id, 0x01));
		virt->frame_seq_no_len(2);

		_map1 = _stack.create_map<map_packet_source>(gmapid_t(virt->channel_id, 0x00));
		_map2 = _stack.create_map<map_access_emitter>(gmapid_t(virt->channel_id, 0x01));
	}

	void send_command();

private:
	ccsds::uslp::map_packet_source * _map1;
	ccsds::uslp::map_access_emitter * _map2;
	ccsds::uslp::output_stack _stack;

};

void ccsds_init_ostack(ccsds::uslp::output_stack & stack)
{

}


int main()
{
	zmq::context_t ctx;
	zmq::socket_t socket(ctx, ZMQ_PAIR);


	return 0;
}
