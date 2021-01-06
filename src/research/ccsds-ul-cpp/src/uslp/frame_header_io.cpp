#include <ccsds/uslp/frame_header_io.hpp>

#include <ccsds/uslp/exceptions.hpp>


namespace ccsds { namespace uslp {

	frame_seq_no_length_option::frame_seq_no_length_option(uint8_t value_)
	{
		value(value_);
	}

	void frame_seq_no_length_option::value(uint8_t value_)
	{
		_value = value_;
	}


	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


	frame_header_writer::frame_header_writer()
	{

	}

	void frame_header_writer::set_option(const frame_header_option & option_)
	{
		if (const auto * option = dynamic_cast<const frame_seq_no_length_option*>(&option_))
		{
			set_option(*option);
		}
		else
		{
			throw einval_exception("unimplemented subclass of frame_header_option passed in set_option");
		}
	}


	void frame_header_writer::get_option(frame_header_option & option_) const
	{
		if (auto * option = dynamic_cast<frame_seq_no_length_option*>(&option_))
		{
			get_option(*option);
		}
		else
		{
			throw einval_exception("unimplemented subclass of frame_header_option passed in get_option");
		}
	}


	void frame_header_writer::set_option(const frame_seq_no_length_option & option)
	{
		_frame_seq_no_len = option.value();
	}


	void frame_header_writer::get_option(frame_seq_no_length_option & option) const
	{
		option.value(_frame_seq_no_len);
	}

}}
