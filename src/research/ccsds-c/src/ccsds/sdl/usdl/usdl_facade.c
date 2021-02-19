/*
 * facade.c
 *
 *  Created on: 16 февр. 2021 г.
 *      Author: HP
 */


#include <ccsds/sdl/usdl/usdl_facade.h>
#include <ccsds/sdl/usdl/usdl_defines.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/map_service_abstract.h>
#include <ccsds/sdl/usdl/mapa_service.h>
#include <ccsds/sdl/usdl/mapp_service.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/pc_generate.h>
#include <ccsds/sdl/usdl/multiplexer.h>


#define CAT(A, B) A##B
#define XCAT(A, B) CAT(A, B)
#define USDL_TEMPL_NAME(size) XCAT(usdl__T_, size)
#define USDL_TEMPL(map_count, frame_size, packet_size) \
	struct { \
		usdl_t usdl; \
		uint8_t buf[map_count][frame_size]; \
		uint8_t packet_buf[packet_size]; \
		uint8_t is_buf_used[map_count + 1]; \
	}

enum layer_t {
	LAYER_MAP,
	LAYER_VC,
	LAYER_MC,
	LAYER_PC,
	LAYER_MX,
};
USDL_TEMPL(10, 100, 300) u;
const static size_t layer_to_size[] = {USDL_MAP_MAX_COUNT, USDL_VC_MAX_COUNT,
		USDL_MC_MAX_COUNT, USDL_PC_MAX_COUNT, USDL_MX_MAX_COUNT};
static map_t map_pool[USDL_MAP_MAX_COUNT];
static vc_t vc_pool[USDL_VC_MAX_COUNT];
static mc_t mc_pool[USDL_MC_MAX_COUNT];
static pc_t pc_pool[USDL_PC_MAX_COUNT];
static struct multiplexer_rr mx_pool[USDL_MX_MAX_COUNT];

#define TOTAL_COUNT (USDL_MX_MAX_COUNT + USDL_PC_MAX_COUNT + USDL_MC_MAX_COUNT + USDL_VC_MAX_COUNT + USDL_MAP_MAX_COUNT)

static uint8_t is_inited[TOTAL_COUNT];

static size_t _find_start(enum layer_t layer) {
	size_t start = 0;
	for (int i = 0; i < layer; i++) {
		start += layer_to_size[i];
	}
	return start;
}
static size_t _find_free(enum layer_t layer) {
	size_t start = _find_start(layer);
	for (int i = start; i < layer_to_size[layer]; i++) {
		if (!is_inited[i]) {
			return i - start;
		}
	}
	return layer_to_size[layer];
}

static mc_t *_find_mc(const pc_t *pc, mc_id_t mc_id) {
	for (int i = 0; i < MC_COUNT; i++) {
		if (pc->mc_arr[i] && pc->mc_arr[i]->mc_id == mc_id) {
			return pc->mc_arr[i];
		}
	}
	return 0;
}
static vc_t *_find_vc(const pc_t *pc, mc_id_t mc_id, vc_id_t vc_id) {
	mc_t *mc = _find_mc(pc, mc_id);
	if (mc) {
		return mc->vc_arr[vc_id];
	}
	return 0;
}
static map_t *_find_map(const pc_t *pc, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id) {
	vc_t *vc = _find_vc(pc, mc_id, vc_id);
	if (vc) {
		return vc->map_arr[map_id];
	}
	return 0;
}

void usdl_pc_init(usdl_t *usdl, const pc_paramaters_t *params, uint8_t *data, size_t size) {
	size_t index = _find_free(LAYER_MC);
	assert(index != layer_to_size[LAYER_PC]);
	pc_init(&pc_pool[index], params, data, size);
	usdl->pc = &pc_pool[index];
}
void usdl_mc_init(usdl_t *usdl, const mc_paramaters_t *params, mc_id_t mc_id) {
	size_t index = _find_free(LAYER_MC);
	assert(index != layer_to_size[LAYER_MC]);
	assert(usdl->pc);
	mc_init(&mc_pool[index], usdl->pc, params, mc_id);
}
void usdl_vc_init(usdl_t *usdl, const vc_parameters_t *params, mc_id_t mc_id, vc_id_t vc_id) {
	size_t index = _find_free(LAYER_VC);
	mc_t *mc = _find_mc(usdl->pc, mc_id);
	assert(index != layer_to_size[LAYER_VC]);
	assert(mc);
	vc_init(&vc_pool[index], mc, params, vc_id);
}
void usdl_mapp_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id, uint8_t *data, size_t size) {
	size_t index = _find_free(LAYER_MAP);
	vc_t *vc = _find_vc(usdl->pc, mc_id, vc_id);
	assert(index != layer_to_size[LAYER_MAP]);
	assert(vc);
	map_t *map = &map_pool[index];
	map->buf_ex.data = data;
	map->buf_ex.max_size = size;
	mapp_init(map, vc, map_id);
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

