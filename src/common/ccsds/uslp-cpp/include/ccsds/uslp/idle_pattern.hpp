#ifndef USLP__DETAIL_IDLE_PATTERN_HPP_
#define USLP__DETAIL_IDLE_PATTERN_HPP_

#include <iterator>
#include <vector>

#include <cassert>
#include <cstdint>
#include <cstddef>


namespace ccsds { namespace uslp {


class idle_pattern_generator
{
public:
	typedef uint8_t value_type;

	class const_iterator: public std::iterator<
		std::input_iterator_tag,
		const idle_pattern_generator::value_type
	>
	{
	public:
		const_iterator() = default;

		const_iterator(const idle_pattern_generator * generator)
			: _gen(generator)
		{}

		const_iterator(const idle_pattern_generator * generator, size_t offset)
			: _gen(generator), _offset(offset)
		{}

		const_iterator operator++()
		{
			_offset++;
			return *this;
		}

		const_iterator operator++(int)
		{
			auto tmp = *this;
			_offset++;
			return tmp;
		}

		bool operator==(const const_iterator & other) const
		{
			return this->_offset == other._offset;
		};

		bool operator!=(const const_iterator & other) const
		{
			return !(*this == other);
		};

		reference operator*() const
		{
			return _value();
		}

	private:
		reference _value() const
		{
			assert(_gen);
			return _gen->pattern()[_offset % _gen->pattern_size()];
		}

		const idle_pattern_generator * const _gen = nullptr;
		size_t _offset = 0;
	};

protected:
	idle_pattern_generator();

public:
	static idle_pattern_generator & instance();

	template<typename UINT8_ITT>
	void assign(UINT8_ITT begin, UINT8_ITT end)
	{
		_pattern.assign(begin, end);
	}

	const uint8_t * pattern() const { return &_pattern.front(); }
	const size_t pattern_size() const { return _pattern.size(); }

	const_iterator begin() const { return const_iterator(this); }
	const_iterator end(size_t len) const { return const_iterator(this, len); }

private:
	std::vector<uint8_t> _pattern;

};


}}



#endif /* USLP__DETAIL_IDLE_PATTERN_HPP_ */
