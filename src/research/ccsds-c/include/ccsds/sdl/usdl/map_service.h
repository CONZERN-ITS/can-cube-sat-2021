/*
 * map_service.h
 *
 *  Created on: 8 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_MAP_SERVICE_H_
#define SDL_USDL_MAP_SERVICE_H_


#include "map_multiplexer.h"
#include "usdl_types.h"
#include <string.h>
#include <assert.h>


int mapp_send(map_t *map, uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos);

int mapa_send();

int map_stream_send();





#endif /* SDL_USDL_MAP_SERVICE_H_ */
