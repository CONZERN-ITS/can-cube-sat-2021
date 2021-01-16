#include <ccsds/uslp/virtual/abstract_vchannel.hpp>

#include <sstream>

#include <ccsds/uslp/exceptions.hpp>

#include <ccsds/uslp/_detail/tf_header.hpp>


namespace ccsds { namespace uslp {



static const uint64_t _mask_for_byte_size(uint8_t mask_len)
{
	// FIXME: Подумать - маску нужной длины можно получить одним изящным сдвигом вправо
	switch (mask_len)
	{
	case 0: return 0x0000000000000000;
	case 1: return 0x00000000000000FF;
	case 2: return 0x000000000000FFFF;
	case 3: return 0x0000000000FFFFFF;
	case 4: return 0x00000000FFFFFFFF;
	case 5: return 0x000000FFFFFFFFFF;
	case 6: return 0x0000FFFFFFFFFFFF;
	case 7: return 0x00FFFFFFFFFFFFFF;
	case 8: return 0xFFFFFFFFFFFFFFFF;
	default: {
		std::stringstream error;
		error << "unable to make bitmask for " << mask_len << " bytes";
		throw einval_exception(error.str());
	}
	}; // case


}


vchannel_source::vchannel_source(gvcid_t gvcid_)
	: gvcid(gvcid_)
{

}


vchannel_source::vchannel_source(gvcid_t gvcid_, uint16_t frame_size_l2_)
	: gvcid(gvcid_), _frame_size_l2(frame_size_l2_)
{

}


void vchannel_source::frame_size_l2(uint16_t value)
{
	_frame_size_l2 = value;
}


void vchannel_source::frame_seq_no_len(uint16_t value)
{
	if (detail::tf_header_t::extended_size_forecast(_frame_seq_no_len) > frame_size_l2())
	{
		std::stringstream error;
		error << "unable to set frame seq no len to " << value << " "
				<< "because it would not fit frame with size " << frame_size_l2();

		throw einval_exception(error.str());
	}
	_frame_seq_no_len = value;
}


uint16_t vchannel_source::tfdf_size() const
{
	return _frame_size_l2 - detail::tf_header_t::extended_size_forecast(_frame_seq_no_len);
}


void vchannel_source::set_frame_seq_no(uint64_t value)
{
	if (value > _mask_for_byte_size(_frame_seq_no_len))
	{
		std::stringstream error;
		error << "unable to set frame_seq_no counter to " << value << " "
				<< "because frame_seq_no_len is set to " << _frame_seq_no_len;
		throw einval_exception(error.str());
	}

	_frame_seq_no = value;
}


uint64_t vchannel_source::get_frame_seq_no() const
{
	return _frame_seq_no;
}


void vchannel_source::increase_frame_seq_no()
{
	_frame_seq_no = _frame_seq_no & _mask_for_byte_size(_frame_seq_no_len);
}



}}
