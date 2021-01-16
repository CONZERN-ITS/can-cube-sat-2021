/*
 * map_service.h
 *
 *  Created on: 8 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_USDL_MAP_SERVICE_H_
#define SDL_USDL_MAP_SERVICE_H_

#include <ccsds/sdl/usdl/map_service.h>
#include <ccsds/sdl/usdl/map_multiplexer.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <string.h>
#include <assert.h>


int mapp_send(map_t *map, uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos) {
#if MAP_BUFFER_SIZE == 0
	return 0;
#else
	assert(map->split_allowed);
	assert(map->size_fixed);
	assert(map->payload_size <= MAP_BUFFER_SIZE);
	map_buffer *buf = (qos == QOS_EXPEDITED ? &map->buf_ex : &map->buf_sc);

	map_params_t params;
	params.qos = qos;
	params.map_id = map->map_id;
	params.rules = TFDZCR_SPAN;
	params.upid = UPID_SP_OR_EP;
	params.fhd = buf->fhd;

	if (pvn != PVN_ENCAPSULATION_PROTOCOL && pvn != PVN_SPACE_PACKET) {
		return 0;
	}

	if (buf->index == map->payload_size) {
		if (map_mx_multiplex(map->map_mx, &params, buf->data, map->payload_size)) {
			params.fhd = buf->fhd = 0;
			buf->index = 0;
		} else {
			return 0;
		}
	}

	if (buf->fhd == (uint16_t)-1) {
		params.fhd = buf->fhd = buf->index;
	}
	size_t data_index = 0;
	size_t count_to_send = 0;

	while (1) {
		count_to_send = map->payload_size - buf->index < size - data_index
				? map->payload_size - buf->index : size - data_index;

		memcpy(&buf->data[buf->index], &data[data_index], count_to_send);
		buf->index += count_to_send;
		data_index += count_to_send;
		if (buf->index == map->payload_size &&
				map_mx_multiplex(map->map_mx, &params, buf->data, count_to_send)) {
			buf->index = 0;
			params.fhd = buf->fhd = (fhd_t)-1;
		} else {
			break;
		}
	}
	return data_index;
#endif
}

int mapa_send();

int map_stream_send();





#endif /* SDL_USDL_MAP_SERVICE_H_ */
