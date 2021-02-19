/*
 * facade_static.c
 *
 *  Created on: 19 февр. 2021 г.
 *      Author: HP
 */
#include <ccsds/sdl/usdl/facade_static.h>

#include <ccsds/sdl/usdl/map_service_abstract.h>
#include <ccsds/sdl/usdl/mapa_service.h>
#include <ccsds/sdl/usdl/mapp_service.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/pc_generate.h>
#include <ccsds/sdl/usdl/multiplexer.h>
#include <ccsds/sdl/usdl/usdl_types.h>

#include <stdio.h>

enum layer_t {
	LAYER_MAP,
	LAYER_VC,
	LAYER_MC,
	LAYER_MX
};

int _take_free(const usdl_t *usdl, enum layer_t layer) {
	size_t end = 0;
	size_t start = 0;
	switch (layer) {
	case LAYER_MAP: end += usdl->map_pool_size; //fallthrough
	case LAYER_VC: end += usdl->vc_pool_size; //fallthrough
	case LAYER_MC: end += usdl->mc_pool_size; //fallthrough
	case LAYER_MX: end += usdl->mx_pool_size; //fallthrough
	}
	switch (layer) {
	case LAYER_MAP: start = end - usdl->map_pool_size; break;
	case LAYER_VC: start = end - usdl->vc_pool_size; break;
	case LAYER_MC: start = end - usdl->mc_pool_size; break;
	case LAYER_MX: start = end - usdl->mx_pool_size; break;
	}

	for (size_t i = start; i < end; i++) {
		if (!usdl->is_occupied[i]) {
			usdl->is_occupied[i] = 1;
			return i - start;
		}
	}
	return -1;
}

mc_t *_find_mc(const usdl_t *usdl, mc_id_t mc_id) {
	pc_t *pc = usdl->pc;
	for (int i = 0; i < MC_COUNT; i++) {
		if (pc->mc_arr[i]->mc_id == mc_id) {
			return pc->mc_arr[i];
		}
	}
	return 0;
}
vc_t *_find_vc(const usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id) {
	mc_t *mc = _find_mc(usdl, mc_id);
	if (mc == 0) {
		return 0;
	} else {
		return mc->vc_arr[vc_id];
	}
}
map_t *_find_map(const usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	vc_t *vc = _find_vc(usdl, mc_id, vc_id);
	if (vc == 0) {
		return 0;
	} else {
		return vc->map_arr[map_id];
	}
}

void usdl_vc_init(usdl_t *usdl, const vc_parameters_t *params, mc_id_t mc_id, vc_id_t vc_id) {
	int index = _take_free(usdl, LAYER_VC);
	int index1 = _take_free(usdl, LAYER_MX);
	mc_t *mc = _find_mc(usdl, mc_id);
	assert(index >= 0);
	assert(index1 >= 0);
	assert(mc);

	vc_init(&usdl->vc_pool[index], mc, params, vc_id);
	usdl->vc_pool[index].map_mx = usdl->mx_pool[index1];
}
void usdl_mc_init(usdl_t *usdl, const mc_paramaters_t *params, mc_id_t mc_id) {
	int index = _take_free(usdl, LAYER_MC);
	int index1 = _take_free(usdl, LAYER_MX);
	assert(index >= 0);
	assert(index1 >= 0);
	assert(usdl->pc);

	mc_init(&usdl->mc_pool[index], usdl->pc, params, mc_id);
	usdl->mc_pool[index].vc_mx = usdl->mx_pool[index1];
}
void usdl_pc_init(usdl_t *usdl, const pc_paramaters_t *params) {
	int index1 = _take_free(usdl, LAYER_MX);
	assert(index1 >= 0);
	pc_init(usdl->pc, params, usdl->frame_buf_pc, usdl->frame_size);
	usdl->pc->mc_mx = usdl->mx_pool[index1];
}
void usdl_mapp_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	int index = _take_free(usdl, LAYER_MAP);
	vc_t *vc = _find_vc(usdl, mc_id, vc_id);
	assert(index >= 0);
	assert(vc);

	map_t *map = &usdl->map_pool[index];
	mapp_init(map, vc, map_id);
	map->buf_ex.data = usdl->frame_buf_pool + index * usdl->frame_size;
	map->buf_ex.max_size = usdl->frame_size;
	map->mapr.packet.data = usdl->packet_buf_pool + index * usdl->packet_size;
	map->mapr.packet.max_size = usdl->packet_size;
}
void usdl_mapa_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	int index = _take_free(usdl, LAYER_MAP);
	vc_t *vc = _find_vc(usdl, mc_id, vc_id);
	assert(index >= 0);
	assert(vc);

	map_t *map = &usdl->map_pool[index];
	mapa_init(map, vc, map_id);
	map->buf_ex.data = usdl->frame_buf_pool + index * usdl->frame_size;
	map->buf_ex.max_size = usdl->frame_size;
	map->mapr.packet.data = usdl->packet_buf_pool + index * usdl->packet_size;
	map->mapr.packet.max_size = usdl->packet_size;
}
sap_t usdl_get_map_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	return _find_map(usdl, mc_id, vc_id, map_id);
}
sap_t usdl_get_vc_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id) {
	return _find_vc(usdl, mc_id, vc_id);
}
sap_t usdl_get_mc_sap(usdl_t *usdl, mc_id_t mc_id) {
	return _find_mc(usdl, mc_id);
}
int usdl_mapp_send(sap_t sap, const uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos) {
	return mapp_send(sap, data, size, pvn, qos);
}
int usdl_mapa_send(sap_t sap, const uint8_t *data, size_t size, quality_of_service_t qos) {
	return mapa_send(sap, data, size, qos);
}
int usdl_mapp_recieve(sap_t sap, uint8_t *data, size_t size, quality_of_service_t *qos) {
	map_t *map = sap;
	return mapp_recieve(&map->mapr, data, size, qos);
}
int usdl_mapa_recieve(sap_t sap, uint8_t *data, size_t size, quality_of_service_t *qos) {
	map_t *map = sap;
	return mapa_recieve(&map->mapr, data, size, qos);
}
void usdl_pc_set_frame_buf_empty(usdl_t *usdl, uint8_t is_frame_buf_empty) {
	usdl->pc->is_valid = !is_frame_buf_empty;
}
uint8_t usdl_pc_is_frame_buf_empty(const usdl_t *usdl) {
	return !usdl->pc->is_valid;
}
int usdl_pc_get_frame(usdl_t *usdl, uint8_t **data, size_t **size) {
	if (!usdl->pc->is_valid) {
		pc_request_from_down(usdl->pc);
	}
	*data = usdl->pc->data;
	*size = &usdl->pc->size;
	return usdl->pc->is_valid;
}
int usdl_pc_set_frame(usdl_t *usdl, uint8_t *data, size_t size) {
	pc_parse(usdl->pc, data, size);
	return usdl->pc->is_valid;

}
