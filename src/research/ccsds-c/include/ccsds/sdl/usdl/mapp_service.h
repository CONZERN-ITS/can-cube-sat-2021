#ifndef SDL_USDL_MAP_SERVICE_H_
#define SDL_USDL_MAP_SERVICE_H_


#include "map_multiplexer.h"
#include "usdl_types.h"
#include <ccsds/sdl/usdl/map_service_abstract.h>
#include <string.h>
#include <assert.h>


int mapp_init(map_t *map, map_mx_t *mx, map_id_t map_id);

int mapp_send(map_t *map, uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos);

//int mapp_request_from_down(map_t *map);

int mapp_recieve(mapr_t *map, uint8_t *data, size_t size, quality_of_service_t *qos);


#endif /* SDL_USDL_MAP_SERVICE_H_ */
