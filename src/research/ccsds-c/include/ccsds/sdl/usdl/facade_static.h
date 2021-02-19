/*
 * facade_static.h
 *
 *  Created on: 19 февр. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_SDL_USDL_FACADE_STATIC_H_
#define INCLUDE_CCSDS_SDL_USDL_FACADE_STATIC_H_


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

typedef void *sap_t;

sap_t usdl_get_map_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);
sap_t usdl_get_vc_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id);
sap_t usdl_get_mc_sap(usdl_t *usdl, mc_id_t mc_id);
int usdl_mapp_push(sap_t sap, const uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos);
int usdl_mapa_push(sap_t sap, const uint8_t *data, size_t size, quality_of_service_t qos);


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


void usdl_vc_init(usdl_t *usdl, const vc_parameters_t *params, mc_id_t mc_id, vc_id_t vc_id);
void usdl_mc_init(usdl_t *usdl, const mc_paramaters_t *params, mc_id_t mc_id);
void usdl_pc_init(usdl_t *usdl, const pc_paramaters_t *params);

void usdl_mapp_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);

void usdl_mapa_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);


#endif /* INCLUDE_CCSDS_SDL_USDL_FACADE_STATIC_H_ */
