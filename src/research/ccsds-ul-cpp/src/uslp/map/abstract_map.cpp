#include <ccsds/uslp/map/abstract_map.hpp>

namespace ccsds { namespace uslp {


map_source::map_source(gmap_id_t map_id_)
	: map_id(map_id_)
{

}


map_source::map_source(gmap_id_t map_id_, uint16_t tfdf_size_)
	: map_id(map_id_), _tfdf_size(tfdf_size_)
{

}


void map_source::tfdf_size(uint16_t value)
{
	_tfdf_size = value;
}


}}
