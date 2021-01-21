/*
 * map_multiplexer.h
 *
 *  Created on: 8 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_MAP_MULTIPLEXER_H_
#define SDL_USDL_MAP_MULTIPLEXER_H_

#include "usdl_types.h"
#include <string.h>
#include <assert.h>
#include "vc_generate.h"

int map_mx_init(map_mx_t *map_mx, vc_t *vc);

int map_mx_multiplex(map_mx_t *map_mx, map_params_t *map_params, uint8_t *data, size_t size);

int map_mx_request_from_down(map_mx_t *map_mx);

#endif /* SDL_USDL_MAP_MULTIPLEXER_H_ */
