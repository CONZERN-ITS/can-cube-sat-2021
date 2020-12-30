#ifndef CCSDS_IDS_HPP_
#define CCSDS_IDS_HPP_

#include <cstddef>
#include <cstdint>

namespace ccsds
{
	typedef uint8_t vcid_t;


	class mcid_t
	{
	public:
		mcid_t() = default;
		mcid_t(uint8_t packet_version_number, uint16_t spacecraft_id);

		uint8_t packet_version_number() const { return _pvn; }
		void packet_version_number(uint8_t value);

		uint16_t spacecraft_id() const { return _sc_id; }
		void spacecraft_id(uint16_t value);

	private:
		uint8_t _pvn = 0;
		uint16_t _sc_id = 0;
	};


	class gvcid_t: public mcid_t
	{
	public:
		gvcid_t() = default;
		gvcid_t(mcid_t mcid, vcid_t vcid);
		gvcid_t(uint8_t packet_version_number, uint16_t spacecraft_id, vcid_t vchannel_id);

		vcid_t vchannel_id() const { return _vch_id; }
		void vchannel_id(vcid_t value);

	private:
		vcid_t _vch_id = 0;
	};

}

#endif /* CCSDS_IDS_HPP_ */
