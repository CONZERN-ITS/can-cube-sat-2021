#include <ccsds/uslp/exceptions.hpp>


namespace ccsds {


exception::exception(const char * what)
	: std::runtime_error(what)
{

}


exception::exception(const std::string & what)
	: std::runtime_error(what)
{

}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


einval_exception::einval_exception(const char * what)
	: exception(what)
{

}


einval_exception::einval_exception(const std::string & what)
	: exception(what)
{

}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


invalid_call_order::invalid_call_order(const char * what)
	: exception(what)
{}


invalid_call_order::invalid_call_order(const std::string & what)
	: exception(what)
{}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


object_is_finalized::object_is_finalized(const char * what)
	: exception(what)
{

}


object_is_finalized::object_is_finalized(const std::string & what)
	: exception(what)
{

}


}


