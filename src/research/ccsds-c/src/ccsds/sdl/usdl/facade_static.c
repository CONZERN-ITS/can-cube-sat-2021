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

typedef struct usdl_t {
	pc_t *pc;
	mc_t *mc_pool;
	size_t mc_pool_size;
	vc_t *vc_pool;
	size_t vc_pool_size;
	map_t *map_pool;
	size_t map_pool_size;
	struct multiplexer_abstract *mx_pool;
	size_t mx_pool_size;
	uint8_t *is_free;
	size_t is_free_size;
	uint8_t *frame_buf_pool;
	size_t frame_size;
	uint8_t *packet_buf_pool;
	size_t packet_size;
} usdl_t;

#define USDL_STATIC_ALLOCATE(map_count, vc_count, mc_count, packet_size, frame_size) \
	struct { \
		usdl_t base; \
		map_t map_pool[map_count]; \
		vc_t vc_pool[vc_count]; \
		mc_t mc_pool[mc_count]; \
		pc_t pc; \
		struct multiplexer_rr mx_pool[(vc_count) + (mc_count)]; \
		uint8_t is_free[(map_count) + (vc_count) + (mc_count) + (vc_count) + (mc_count)]; \
		uint8_t frame_buf_pool[(map_count) * (frame_size)]; \
		uint8_t packet_buf_pool[(1 + (vc_count)) * (packet_size)]; \
	}
#define USDL_STATIC_INIT(usdl_static, usdl) do { \
		usdl.pc = &usdl_static.pc; \
		usdl.mc_pool = usdl_static.mc_pool; \
		usdl.vc_pool = usdl_static.vc_pool; \
		usdl.map_pool = usdl_static.map_pool; \
		usdl.mx_pool = (struct multiplexer_abstract *)usdl_static.mx_pool; \
		usdl.frame_buf_pool = usdl_static.frame_buf_pool; \
		usdl.packet_buf_pool = usdl_static.packet_buf_pool; \
		\
		usdl.mc_pool_size = sizeof(usdl_static.mc_pool) / sizeof(usdl_static.mc_pool[0]); \
		usdl.vc_pool_size = sizeof(usdl_static.vc_pool) / sizeof(usdl_static.vc_pool[0]); \
		usdl.map_pool_size = sizeof(usdl_static.map_pool) / sizeof(usdl_static.map_pool[0]); \
		usdl.mx_pool_size = sizeof(usdl_static.mx_pool) / sizeof(usdl_static.mx_pool[0]); \
	} while (0);
#define usdl_static_TEMPLATE_TO_usdl_static(usdl_static) usdl_static.base;

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
