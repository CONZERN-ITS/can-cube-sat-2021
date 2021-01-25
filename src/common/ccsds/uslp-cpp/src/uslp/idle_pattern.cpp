#include <ccsds/uslp/idle_pattern.hpp>

#include <algorithm>


namespace ccsds { namespace uslp {


static const uint8_t _default_pattern[] = { 'I', 'D', 'L', 'E' };


idle_pattern_generator::idle_pattern_generator()
{
	_pattern.assign(std::cbegin(_default_pattern), std::cend(_default_pattern));
}


idle_pattern_generator & idle_pattern_generator::instance()
{
	static idle_pattern_generator retval;
	return retval;
}


}}



