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
	struct multiplexer_abstract **mx_pool;
	size_t mx_pool_size;
	uint8_t *is_occupied;
	size_t is_occupied_size;
	uint8_t *frame_buf_pool;
	size_t frame_size;
	uint8_t *packet_buf_pool;
	size_t packet_size;
	uint8_t *frame_buf_pc;
} usdl_t;

typedef void *sap_t;

sap_t usdl_get_map_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);
sap_t usdl_get_vc_sap(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id);
sap_t usdl_get_mc_sap(usdl_t *usdl, mc_id_t mc_id);
int usdl_mapp_send(sap_t sap, const uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos);
int usdl_mapa_send(sap_t sap, const uint8_t *data, size_t size, quality_of_service_t qos);
int usdl_mapp_recieve(sap_t sap, uint8_t *data, size_t size, quality_of_service_t *qos);
int usdl_mapa_recieve(sap_t sap, uint8_t *data, size_t size, quality_of_service_t *qos);


#define USDL_STATIC_ALLOCATE(map_count, vc_count, mc_count, packet_size, frame_size) \
	struct { \
		usdl_t base; \
		map_t map_pool[map_count]; \
		vc_t vc_pool[vc_count]; \
		mc_t mc_pool[mc_count]; \
		pc_t pc; \
		struct multiplexer_rr mx_pool_im[(vc_count) + (mc_count)]; \
		struct multiplexer_abstract *mx_pool[(vc_count) + (mc_count)]; \
		uint8_t is_occupied[(map_count) + (vc_count) + (mc_count) + (vc_count) + (mc_count)]; \
		uint8_t frame_buf_pc[frame_size]; \
		uint8_t frame_buf_pool[(map_count) * (frame_size)]; \
		uint8_t packet_buf_pool[(map_count) * (packet_size)]; \
	}
#define USDL_STATIC_INIT(usdl_static, usdl) do { \
		usdl.pc = &usdl_static.pc; \
		usdl.mc_pool = usdl_static.mc_pool; \
		usdl.vc_pool = usdl_static.vc_pool; \
		usdl.map_pool = usdl_static.map_pool; \
		usdl.mx_pool = usdl_static.mx_pool; \
		usdl.frame_buf_pool = usdl_static.frame_buf_pool; \
		usdl.packet_buf_pool = usdl_static.packet_buf_pool; \
		usdl.frame_buf_pc = usdl_static.frame_buf_pc; \
		usdl.is_occupied = usdl_static.is_occupied; \
		\
		usdl.is_occupied_size = sizeof(usdl_static.is_occupied); \
		usdl.mc_pool_size = sizeof(usdl_static.mc_pool) / sizeof(usdl_static.mc_pool[0]); \
		usdl.vc_pool_size = sizeof(usdl_static.vc_pool) / sizeof(usdl_static.vc_pool[0]); \
		usdl.map_pool_size = sizeof(usdl_static.map_pool) / sizeof(usdl_static.map_pool[0]); \
		usdl.mx_pool_size = sizeof(usdl_static.mx_pool) / sizeof(usdl_static.mx_pool[0]); \
		usdl.frame_size = sizeof(usdl_static.frame_buf_pc); \
		usdl.packet_size = sizeof(usdl_static.packet_buf_pool) / usdl.map_pool_size; \
		\
		for (size_t i = 0; i < usdl.mx_pool_size; i++) {\
			usdl.mx_pool[i] = (struct multiplexer_abstract *)&usdl_static.mx_pool_im[i]; \
			mx_construct_multiplexer_rr(&usdl_static.mx_pool_im[i]); \
		} \
	} while (0);


void usdl_vc_init(usdl_t *usdl, const vc_parameters_t *params, mc_id_t mc_id, vc_id_t vc_id);
void usdl_mc_init(usdl_t *usdl, const mc_paramaters_t *params, mc_id_t mc_id);
void usdl_pc_init(usdl_t *usdl, const pc_paramaters_t *params);

void usdl_mapp_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);

void usdl_mapa_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id);


void usdl_pc_set_frame_buf_empty(usdl_t *usdl, uint8_t is_frame_buf_empty);
uint8_t usdl_pc_is_frame_buf_empty(const usdl_t *usdl);
int usdl_pc_get_frame(usdl_t *usdl, uint8_t **data, size_t **size);
int usdl_pc_set_frame(usdl_t *usdl, uint8_t *data, size_t size);

#endif /* INCLUDE_CCSDS_SDL_USDL_FACADE_STATIC_H_ */
