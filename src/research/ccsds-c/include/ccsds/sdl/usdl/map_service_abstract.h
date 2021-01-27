#ifndef INCLUDE_CCSDS_SDL_USDL_MAP_SERVICE_ABSTRACT_H_
#define INCLUDE_CCSDS_SDL_USDL_MAP_SERVICE_ABSTRACT_H_

#include <ccsds/sdl/usdl/usdl_types.h>

static inline void map_parse(map_t *map, uint8_t *data, size_t size,
		map_params_t *map_params) {
	map->map_parse(map, data, size, map_params);
}

static inline int map_request_from_down(map_t *map) {
	return map->map_request_from_down(map);
}

#endif /* INCLUDE_CCSDS_SDL_USDL_MAP_SERVICE_ABSTRACT_H_ */
