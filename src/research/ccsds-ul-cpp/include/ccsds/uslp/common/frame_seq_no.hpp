#ifndef INCLUDE_CCSDS_USLP__COMMON_FRAME_SEQ_NO_HPP_
#define INCLUDE_CCSDS_USLP__COMMON_FRAME_SEQ_NO_HPP_

#include <cstdint>

namespace ccsds { namespace uslp {

class frame_seq_no_t
{
public:
	frame_seq_no_t() = default;
	frame_seq_no_t(uint64_t value_, uint8_t value_size_) { value(value_, value_size_); }

	const frame_seq_no_t & operator++();
	frame_seq_no_t operator++(int);

	const frame_seq_no_t & operator--();
	frame_seq_no_t operator--(int);

	frame_seq_no_t & operator+(uint64_t increment);
	frame_seq_no_t & operator-(uint64_t decrement);

	const bool operator==(const frame_seq_no_t & other);
	const bool operator!=(const frame_seq_no_t & other);

	void value(uint64_t value_, uint8_t value_size_);
	uint64_t value() const { return _value; }
	uint8_t value_size() const { return _value_size; }


private:
	uint64_t _value = 0;
	uint8_t _value_size = 1;
};

}}

#endif /* INCLUDE_CCSDS_USLP__COMMON_FRAME_SEQ_NO_HPP_ */
