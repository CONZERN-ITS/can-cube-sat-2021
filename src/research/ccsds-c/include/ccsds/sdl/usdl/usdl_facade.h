/*
 * usdl_facade.h
 *
 *  Created on: 16 февр. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_SDL_USDL_USDL_FACADE_H_
#define INCLUDE_CCSDS_SDL_USDL_USDL_FACADE_H_

#include <ccsds/sdl/usdl/usdl_basic_types.h>
#include <ccsds/sdl/usdl/usdl_types.h>
typedef struct {
	map_id_t map_id;
	vc_id_t vc_id;
	mc_id_t mc_id;
	sc_id_t sc_id;
} gvcid_t;

typedef struct usdl_t {
	pc_t *pc;
} usdl_t;

enum service_t {
	USDL_MAPP,
	USDL_MAPA,
	USDL_MAPO,
	USDL_VCF,
	USDL_MCF,
	USDL_OCF,
	USDL_INSERT
};

typedef void *sap_t;

void usdl_vc_init(usdl_t *usdl, const vc_parameters_t *params, mc_id_t mc_id, vc_id_t vc_id);
void usdl_mapp_init(usdl_t *usdl, mc_id_t mc_id, vc_id_t vc_id, map_id_t map_id, uint8_t *data, size_t size);
sap_t usdl_get_map_sap(usdl_t *usdl, mc_id_t, vc_id_t, map_id_t);


#endif /* INCLUDE_CCSDS_SDL_USDL_USDL_FACADE_H_ */
