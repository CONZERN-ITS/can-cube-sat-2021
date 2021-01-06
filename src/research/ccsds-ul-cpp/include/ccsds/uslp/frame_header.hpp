#ifndef INCLUDE_CCSDS_USLP_FRAME_HEADER_HPP_
#define INCLUDE_CCSDS_USLP_FRAME_HEADER_HPP_

#include <cstdint>
#include <cstddef>

#include <ccsds/uslp/ids.hpp>
#include <ccsds/uslp/defs.hpp>


namespace ccsds { namespace uslp {

	enum class frame_header_kind_t
	{
		TRUNCATED,
		COMPLETE,
	};


	class frame_header
	{
	public:
		virtual ~frame_header() = default;

		const frame_header_kind_t frame_header_kind;

	protected:
		frame_header(frame_header_kind_t hdr_kind)
			: frame_header_kind(hdr_kind)
		{}

	};


	class truncated_frame_header: public frame_header
	{
	public:
		truncated_frame_header();
		virtual ~truncated_frame_header() = default;

		void gmap_id(const gmap_id_t & value) { _gmap_id = value; }
		gvcid_t gmap_id() const { return _gmap_id; }

		void destination_flag(bool value) { _destination_frame = value; }
		bool destination_flag() const { return _destination_frame; }

	protected:
		truncated_frame_header(frame_header_kind_t frame_header_kind_override);

	private:
		bool _destination_frame = false;
		gmap_id_t _gmap_id;
	};


	class complete_frame_header: public truncated_frame_header
	{
	public:
		complete_frame_header();
		virtual ~complete_frame_header() = default;

		void frame_len(uint16_t value) { _frame_len = value; }
		uint16_t frame_len() const { return _frame_len; }

		void frame_class(frame_class_t value) { _frame_class = value; }
		frame_class_t frame_class() const { return _frame_class; }

		void ocf_present(bool value) { _ocf_present = value; }
		bool ocf_present() const { return _ocf_present; }

		void frame_seq_no(uint64_t value) { _frame_seq_no = value; }
		uint64_t frame_seq_no() const { return _frame_seq_no; }

	private:
		frame_class_t _frame_class = frame_class_t::CONTROLLED_PAYLOAD;
		uint16_t _frame_len = 0;
		bool _ocf_present = false;
		uint64_t _frame_seq_no;
	};

}}


#endif /* INCLUDE_CCSDS_USLP_FRAME_HEADER_HPP_ */
