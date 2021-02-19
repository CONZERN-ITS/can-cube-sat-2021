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
	case LAYER_VC: start = end - usdl->map_pool_size; break;
	case LAYER_MC: start = end - usdl->map_pool_size; break;
	case LAYER_MX: start = end - usdl->map_pool_size; break;
	}

	for (size_t i = start; i < end; i++) {
		if (usdl->is_free[i]) {
			usdl->is_free[i] = 0;
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
	mc_t *mc = _find_mc(usdl, mc_id);
	assert(index >= 0);
	assert(mc);

	vc_init(&usdl->vc_pool[index], &mc, params, vc_id);
}
void usdl_mc_init(usdl_t *usdl, const mc_paramaters_t *params, mc_id_t mc_id) {
	int index = _take_free(usdl, LAYER_MC);
	assert(index >= 0);
	assert(usdl->pc);

	mc_init(&usdl->vc_pool[index], usdl->pc, params, mc_id);
}
void usdl_pc_init(usdl_t *usdl, const pc_paramaters_t *params) {
	pc_init(usdl->pc, params, usdl->packet_buf_pool + (usdl->packet_size * usdl->vc_pool_size), usdl->packet_size);
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

void usdl_init() {
	USDL_STATIC_ALLOCATE(10, 4, 1, 1000, 100) usdl_static;
	usdl_t usdl = {0};
	USDL_STATIC_INIT(usdl_static, usdl);
}
sap_t usdl_get_map_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	return _find_map(usdl->pc, mc_id, vc_id, map_id);
}
sap_t usdl_get_vc_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id) {
	return _find_vc(usdl->pc, mc_id, vc_id);
}
sap_t usdl_get_mc_sap(usdl_t *usdl, mc_id_t mc_id) {
	return _find_mc(usdl->pc, mc_id);
}
int usdl_mapp_push(sap_t sap, const uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos) {
	return mapp_send(sap, data, size, pvn, qos);
}
int usdl_mapa_push(sap_t sap, const uint8_t *data, size_t size, quality_of_service_t qos) {
	return mapa_send(sap, data, size, qos);
}
