#include <ccsds/ids.hpp>

#include <stdexcept>


namespace ccsds
{
	mcid_t::mcid_t(uint8_t packet_version_number_, uint16_t spacecraft_id_)
	{
		packet_version_number(packet_version_number_);
		spacecraft_id(spacecraft_id_);
	}


	void mcid_t::packet_version_number(uint8_t value)
	{
		if (value > 0x03)
			throw std::runtime_error("invalid ccsds packet version number");

		this->_pvn = value;
	}


	void mcid_t::spacecraft_id(uint16_t value)
	{
		if (value > 0x3FF)
			throw std::runtime_error("invalid ccsds spacecraft id");

		this->_sc_id = value;
	}



	gvcid_t::gvcid_t(mcid_t mcid, vcid_t vcid)
	: mcid_t(mcid), _vch_id(vcid)
	{
		vchannel_id(vcid);
	}


	gvcid_t::gvcid_t(uint8_t packet_version_number, uint16_t spacecraft_id, uint8_t vchannel_id_)
		: mcid_t(packet_version_number, spacecraft_id)
	{
		vchannel_id(vchannel_id_);
	}


	void gvcid_t::vchannel_id(uint8_t value)
	{
		if (value > 0x3F)
			throw std::runtime_error("invalid cccsds telecommand gvcid virtual channel id");

		this->_vch_id = value;
	}

}
