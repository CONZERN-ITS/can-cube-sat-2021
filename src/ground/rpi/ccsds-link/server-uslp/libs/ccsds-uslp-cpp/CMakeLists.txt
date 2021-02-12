
set(_remote_src_dir ${CMAKE_CURRENT_LIST_DIR}/remote)

add_library(ccsds-uslp-cpp STATIC
	"${_remote_src_dir}/include/ccsds/epp/epp_header.hpp"
	"${_remote_src_dir}/src/epp/epp_header.cpp"
	
	"${_remote_src_dir}/include/ccsds/uslp/output_stack.hpp"
	"${_remote_src_dir}/src/uslp/output_stack.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/input_stack.hpp"
	"${_remote_src_dir}/src/uslp/input_stack.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/common/rr_muxer.hpp"
	"${_remote_src_dir}/include/ccsds/uslp/common/ids.hpp"
	"${_remote_src_dir}/include/ccsds/uslp/common/defs.hpp"
	"${_remote_src_dir}/include/ccsds/uslp/common/frame_seq_no.hpp"
	"${_remote_src_dir}/src/uslp/common/frame_seq_no.cpp"
	
	"${_remote_src_dir}/include/ccsds/uslp/exceptions.hpp"
	"${_remote_src_dir}/src/uslp/exceptions.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/idle_pattern.hpp"
	"${_remote_src_dir}/src/uslp/idle_pattern.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/_detail/tfdf_header.hpp"
	"${_remote_src_dir}/src/uslp/_detail/tfdf_header.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/_detail/tf_header.hpp"
	"${_remote_src_dir}/src/uslp/_detail/tf_header.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/map/abstract_map.hpp"
	"${_remote_src_dir}/src/uslp/map/abstract_map.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/map/map_access_acceptor.hpp"
	"${_remote_src_dir}/src/uslp/map/map_access_acceptor.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/map/map_access_emitter.hpp"
	"${_remote_src_dir}/src/uslp/map/map_access_emitter.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/map/map_packet_acceptor.hpp"
	"${_remote_src_dir}/src/uslp/map/map_packet_acceptor.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/map/map_packet_emitter.hpp"
	"${_remote_src_dir}/src/uslp/map/map_packet_emitter.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/virtual/abstract_vchannel.hpp"
	"${_remote_src_dir}/src/uslp/virtual/abstract_vchannel.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/virtual/map_demuxer.hpp"
	"${_remote_src_dir}/src/uslp/virtual/map_demuxer.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/virtual/map_rr_muxer.hpp"
	"${_remote_src_dir}/src/uslp/virtual/map_rr_muxer.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/master/abstract_mchannel.hpp"
	"${_remote_src_dir}/src/uslp/master/abstract_mchannel.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/master/vchannel_demuxer.hpp"
	"${_remote_src_dir}/src/uslp/master/vchannel_demuxer.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/master/vchannel_rr_muxer.hpp"
	"${_remote_src_dir}/src/uslp/master/vchannel_rr_muxer.cpp"

	"${_remote_src_dir}/include/ccsds/uslp/physical/abstract_pchannel.hpp"
	"${_remote_src_dir}/src/uslp/physical/abstract_pchannel.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/physical/mchannel_demuxer.hpp"
	"${_remote_src_dir}/src/uslp/physical/mchannel_demuxer.cpp"
	"${_remote_src_dir}/include/ccsds/uslp/physical/mchannel_rr_muxer.hpp"
	"${_remote_src_dir}/src/uslp/physical/mchannel_rr_muxer.cpp"
)

target_include_directories(ccsds-uslp-cpp PUBLIC ${_remote_src_dir}/include)
target_compile_features(ccsds-uslp-cpp PUBLIC cxx_std_17)

