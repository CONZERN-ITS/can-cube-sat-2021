cmake_minimum_required(VERSION 3.17)

project(ccsds-uslp-cpp)

# set (CMAKE_CXX_STANDARD 17)

set(_src_dir ${CMAKE_CURRENT_LIST_DIR})
add_library(ccsds-uslp-cpp STATIC
	"${_src_dir}/include/ccsds/epp/epp_header.hpp"
	"${_src_dir}/src/epp/epp_header.cpp"

	"${_src_dir}/include/ccsds/uslp/common/frame_seq_no.hpp"
	"${_src_dir}/src/uslp/common/frame_seq_no.cpp"
	"${_src_dir}/include/ccsds/uslp/common/rr_muxer.hpp"

	"${_src_dir}/include/ccsds/uslp/ids.hpp"
	"${_src_dir}/include/ccsds/uslp/defs.hpp"
	"${_src_dir}/include/ccsds/uslp/exceptions.hpp"
	"${_src_dir}/src/uslp/exceptions.cpp"
	"${_src_dir}/include/ccsds/uslp/idle_pattern.hpp"
	"${_src_dir}/src/uslp/idle_pattern.cpp"

	"${_src_dir}/include/ccsds/uslp/_detail/tfdf_header.hpp"
	"${_src_dir}/src/uslp/_detail/tfdf_header.cpp"
	"${_src_dir}/include/ccsds/uslp/_detail/tf_header.hpp"
	"${_src_dir}/src/uslp/_detail/tf_header.cpp"

	"${_src_dir}/include/ccsds/uslp/map/abstract_map.hpp"
	"${_src_dir}/src/uslp/map/abstract_map.cpp"
	"${_src_dir}/include/ccsds/uslp/map/map_access_sink.hpp"
	"${_src_dir}/src/uslp/map/map_access_sink.cpp"
	"${_src_dir}/include/ccsds/uslp/map/map_access_source.hpp"
	"${_src_dir}/src/uslp/map/map_access_source.cpp"
	"${_src_dir}/include/ccsds/uslp/map/map_packet_sink.hpp"
	"${_src_dir}/src/uslp/map/map_packet_sink.cpp"
	"${_src_dir}/include/ccsds/uslp/map/map_packet_source.hpp"
	"${_src_dir}/src/uslp/map/map_packet_source.cpp"

	"${_src_dir}/include/ccsds/uslp/virtual/abstract_vchannel.hpp"
	"${_src_dir}/src/uslp/virtual/abstract_vchannel.cpp"
	"${_src_dir}/include/ccsds/uslp/virtual/map_demuxer.hpp"
	"${_src_dir}/src/uslp/virtual/map_demuxer.cpp"
	"${_src_dir}/include/ccsds/uslp/virtual/map_rr_muxer.hpp"
	"${_src_dir}/src/uslp/virtual/map_rr_muxer.cpp"

	"${_src_dir}/include/ccsds/uslp/master/abstract_mchannel.hpp"
	"${_src_dir}/src/uslp/master/abstract_mchannel.cpp"
	"${_src_dir}/include/ccsds/uslp/master/vchannel_demuxer.hpp"
	"${_src_dir}/src/uslp/master/vchannel_demuxer.cpp"
	"${_src_dir}/include/ccsds/uslp/master/vchannel_rr_muxer.hpp"
	"${_src_dir}/src/uslp/master/vchannel_rr_muxer.cpp"

	"${_src_dir}/include/ccsds/uslp/physical/abstract_pchannel.hpp"
	"${_src_dir}/src/uslp/physical/abstract_pchannel.cpp"
	"${_src_dir}/include/ccsds/uslp/physical/mchannel_demuxer.hpp"
	"${_src_dir}/src/uslp/physical/mchannel_demuxer.cpp"
	"${_src_dir}/include/ccsds/uslp/physical/mchannel_rr_muxer.hpp"
	"${_src_dir}/src/uslp/physical/mchannel_rr_muxer.cpp"
)


target_include_directories(ccsds-uslp-cpp PUBLIC ${_src_dir}/include)