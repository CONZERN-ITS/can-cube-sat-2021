#ifndef INCLUDE_CCSDS_USLP_IDS_HPP_
#define INCLUDE_CCSDS_USLP_IDS_HPP_

#include <cstdint>

namespace ccsds { namespace uslp {


	class mcid_t
	{
	public:
		mcid_t() = default;
		mcid_t(uint16_t sc_id_)
		{
			sc_id(sc_id_);
		}

		void sc_id(uint16_t value) { _sc_id = value; }
		uint16_t sc_id() const { return _sc_id; }

		const bool operator == (const mcid_t & other) const
		{
			return this->_sc_id == other._sc_id;
		}

		const bool operator != (const mcid_t & other) const
		{
			return !(*this == other);
		}

		const bool operator < (const mcid_t & other) const
		{
			return this->_sc_id < other._sc_id;
		}

	private:
		uint16_t _sc_id = 0;
	};


	class gvcid_t: public mcid_t
	{
	public:
		gvcid_t() = default;
		gvcid_t(uint16_t sc_id_, uint8_t vchannel_id_)
			: mcid_t(sc_id_)
		{
			vchannel_id(vchannel_id_);
		}

		mcid_t mcid() const { return mcid_t(*this); }

		void vchannel_id(uint8_t value) { _vchannel_id = value; }
		uint8_t vchannel_id() const { return _vchannel_id; }

		const bool operator == (const gvcid_t & other) const
		{
			return mcid_t::operator==(other) && this->_vchannel_id == other._vchannel_id;
		}

		const bool operator != (const gvcid_t & other) const
		{
			return !(*this == other);
		}

		const bool operator < (const gvcid_t & other) const
		{
			if (mcid_t::operator==(other))
				return this->_vchannel_id < other._vchannel_id;
			else
				return mcid_t::operator<(other);
		}

	private:
		uint8_t _vchannel_id = 0;
	};


	class gmapid_t: public gvcid_t
	{
	public:
		gmapid_t() = default;
		gmapid_t(uint16_t sc_id_, uint8_t vchannel_id_, uint8_t map_id_)
			: gvcid_t(sc_id_, vchannel_id_)
		{
			map_id(map_id_);
		}

		gvcid_t gvcid() const { return gvcid_t(*this); }

		void map_id(uint8_t value) { _map_id = value; }
		uint8_t map_id() const { return _map_id; }

		const bool operator == (const gmapid_t & other) const
		{
			return gvcid_t::operator==(other) && this->_map_id == other._map_id;
		}

		const bool operator != (const gmapid_t & other) const
		{
			return !(*this == other);
		}

		const bool operator < (const gmapid_t & other) const
		{
			if (gvcid_t::operator==(other))
				return this->_map_id < other._map_id;
			else
				return gvcid_t::operator<(other);
		}

	private:
		uint8_t _map_id = 0;
	};

}}

#endif /* INCLUDE_CCSDS_USLP_IDS_HPP_ */
