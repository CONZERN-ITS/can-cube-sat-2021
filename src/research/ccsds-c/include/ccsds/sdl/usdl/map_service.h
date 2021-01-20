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

typedef enum {
	MAPR_STATE_BEGIN,
	MAPR_STATE_SIZE_UNKNOWN,
	MAPR_STATE_SIZE_KNOWN,
	MAPR_STATE_FINISH
} mapr_state_t;

typedef struct mapr_t {
	map_buffer_t packet;
	map_buffer_t tfdz;
	mapr_state_t state;
} mapr_t;


int mapp_send(map_t *map, uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos);

int mapa_send();

int map_stream_send();

int map_request_from_down(map_t *map);

int map_recieve(mapr_t *map, uint8_t *data, size_t size, quality_of_service_t *qos);




#endif /* SDL_USDL_MAP_SERVICE_H_ */
