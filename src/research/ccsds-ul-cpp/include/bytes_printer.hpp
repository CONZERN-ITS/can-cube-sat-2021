#ifndef INCLUDE_BYTES_PRINTER_HPP_
#define INCLUDE_BYTES_PRINTER_HPP_



template<typename ITT>
class bytes_printer_t
{
public:
	bytes_printer_t(ITT begin, ITT end): _begin(begin), _end(end) {}

	template <typename ITT_2>
	friend std::ostream & operator<<(std::ostream & stream, const bytes_printer_t<ITT_2> & me);
private:
	ITT _begin;
	ITT _end;
};

template<typename ITT_2>
std::ostream & operator<<(std::ostream & stream, const bytes_printer_t<ITT_2> & me)
{
	stream << "0x";
	auto fmt_flags = stream.flags();
	std::exception_ptr error = nullptr;
	try
	{
		stream << std::hex;
		for (auto itt = me._begin; itt != me._end; itt++)
			stream << std::setfill('0') << std::setw(2) << (int)*itt;
	}
	catch (...)
	{
		error = std::current_exception();
	}
	std::cout.flags(fmt_flags);
	if (error) std::rethrow_exception(error);

	return stream;
}


template <typename CONTAINER>
bytes_printer_t<
	typename std::remove_reference<
		decltype(std::cbegin(std::declval<CONTAINER>()))
	>::type
>
print_bytes(const CONTAINER & container)
{
	return bytes_printer_t<decltype(std::cbegin(container))>(
			std::cbegin(container), std::cend(container)
	);
}




#endif /* INCLUDE_BYTES_PRINTER_HPP_ */
