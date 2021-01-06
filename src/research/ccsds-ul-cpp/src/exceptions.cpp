#include <ccsds/uslp/exceptions.hpp>

namespace ccsds { namespace uslp {

	exception::exception(const char * what)
		: std::runtime_error(what)
	{

	}


	exception::exception(const std::string & what)
		: std::runtime_error(what)
	{

	}


	einval_exception::einval_exception(const char * what)
		: exception(what)
	{

	}


	einval_exception::einval_exception(const std::string & what)
		: exception(what)
	{

	}


}}


