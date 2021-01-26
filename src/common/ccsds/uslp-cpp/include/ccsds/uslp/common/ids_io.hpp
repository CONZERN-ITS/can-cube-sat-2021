#ifndef CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_COMMON_IDS_IO_HPP_
#define CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_COMMON_IDS_IO_HPP_


#include <ostream>
#include <iomanip>

#include <ccsds/uslp/common/ids.hpp>


namespace ccsds { namespace uslp {



template<typename CHAR_TYPE, typename TRAITS_TYPE>
std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & operator<<(
		std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & stream, const mcid_t & mcid
)
{
	auto flags = stream.flags();
	try
	{
		stream << "mcid(0x"
				<< std::hex << std::setw(4) << std::setfill('0') << mcid.sc_id()
				<< ")"
		;
		stream.flags(flags);
	}
	catch (...)
	{
		stream.flags(flags);
	}

	return stream;
}


template<typename CHAR_TYPE, typename TRAITS_TYPE>
std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & operator<<(
		std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & stream, const gvcid_t & gvcid
)
{
	auto flags = stream.flags();
	try
	{
		stream << "gvcid("
				<< "0x" << std::hex << std::setw(4) << std::setfill('0') << gvcid.sc_id() << ", "
				<< "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)gvcid.vchannel_id()
				<< ")"
		;
		stream.flags(flags);
	}
	catch (...)
	{
		stream.flags(flags);
	}

	return stream;
}



template<typename CHAR_TYPE, typename TRAITS_TYPE>
std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & operator<<(
		std::basic_ostream<CHAR_TYPE, TRAITS_TYPE> & stream, const gmapid_t & gmapid
)
{
	auto flags = stream.flags();
	try
	{
		stream << "gmapid("
				<< "0x" << std::hex << std::setw(4) << std::setfill('0') << gmapid.sc_id() << ", "
				<< "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)gmapid.vchannel_id()
				<< "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)gmapid.map_id()
				<< ")"
		;
		stream.flags(flags);
	}
	catch (...)
	{
		stream.flags(flags);
	}

	return stream;
}


}}


#endif /* CCSDS_USLP_CPP_INCLUDE_CCSDS_USLP_COMMON_IDS_IO_HPP_ */
