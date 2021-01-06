#ifndef INCLUDE_CCSDS_USLP_FRAME_HEADER_IO_HPP_
#define INCLUDE_CCSDS_USLP_FRAME_HEADER_IO_HPP_


#include <ccsds/uslp/frame_header.hpp>


namespace ccsds { namespace uslp {


	class frame_header_option
	{
	protected:
		frame_header_option() = default;

	public:
		virtual ~frame_header_option() = default;

	};


	class frame_seq_no_length_option
	{
	public:
		frame_seq_no_length_option() = default;
		frame_seq_no_length_option(uint8_t value_);
		virtual ~frame_seq_no_length_option() = default;

		void value(uint8_t value_);
		uint8_t value() const { return _value; }

	private:
		uint8_t _value = 0;
	};


	class frame_header_writer
	{
	public:
		frame_header_writer();

		void set_option(const frame_header_option & option);
		void get_option(frame_header_option & option) const;

		void set_option(const frame_seq_no_length_option & option);
		void get_option(frame_seq_no_length_option & option) const;

	private:
		uint8_t _frame_seq_no_len = 0;
	};

}}



#endif /* INCLUDE_CCSDS_USLP_FRAME_HEADER_IO_HPP_ */
