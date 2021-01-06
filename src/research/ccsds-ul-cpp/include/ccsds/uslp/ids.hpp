#ifndef INCLUDE_CCSDS_USLP_IDS_HPP_
#define INCLUDE_CCSDS_USLP_IDS_HPP_

#include <cstdint>

namespace ccsds { namespace uslp {

	class mcid_t
	{
	public:
		mcid_t() = default;
		mcid_t(uint8_t fvn_, uint16_t sc_id_)
		{
			frame_version_number(fvn_);
			sc_id(sc_id_);
		}

		void frame_version_number(uint8_t value) { _fvn = value; }
		uint8_t frame_version_number() const { return _fvn; }

		void sc_id(uint16_t value) { _sc_id = value; }
		uint16_t sc_id() const { return _sc_id; }

	private:
		uint8_t _fvn = 0;
		uint16_t _sc_id = 0;
	};


	class gvcid_t: public mcid_t
	{
	public:
		gvcid_t() = default;
		gvcid_t(uint8_t fvn_, uint16_t sc_id_, uint8_t vchannel_id_)
			: mcid_t(fvn_, sc_id_)
		{
			vchannel_id(vchannel_id_);
		}

		void vchannel_id(uint8_t value) { _vchannel_id = value; }
		uint8_t vchannel_id() const { return _vchannel_id; }

	private:
		uint8_t _vchannel_id = 0;
	};


	class gmap_id_t: public gvcid_t
	{
	public:
		gmap_id_t() = default;
		gmap_id_t(uint8_t fvn_, uint16_t sc_id_, uint8_t vchannel_id_, uint8_t map_id_)
			: gvcid_t(fvn_, sc_id_, vchannel_id_)
		{
			map_id(map_id_);
		}

		void map_id(uint8_t value) { _map_id = value; }
		uint8_t map_id() const { return _map_id; }

	private:

		uint8_t _map_id = 0;
	};

}}

#endif /* INCLUDE_CCSDS_USLP_IDS_HPP_ */
