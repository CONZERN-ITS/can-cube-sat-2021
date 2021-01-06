#include <ccsds/uslp/frame_header.hpp>

namespace ccsds { namespace uslp {

	truncated_frame_header::truncated_frame_header()
		: frame_header(frame_header_kind_t::TRUNCATED)
	{

	}


	truncated_frame_header::truncated_frame_header(frame_header_kind_t frame_header_kind_override)
		: frame_header(frame_header_kind_override)
	{

	}


	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


	complete_frame_header::complete_frame_header()
		: truncated_frame_header(frame_header_kind_t::COMPLETE)
	{

	}


}}
