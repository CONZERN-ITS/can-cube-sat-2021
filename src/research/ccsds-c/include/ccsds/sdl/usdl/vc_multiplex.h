/*
 * vc_multiplex.h
 *
 *  Created on: 15 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_MULTIPLEX_H_
#define SDL_USDL_VC_MULTIPLEX_H_

#include "usdl_types.h"
#include "mc_generate.h"

int vc_mx_init(vc_mx_t *vc_mx, mc_t *mc);

int vc_multiplex(vc_mx_t *vc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);

int vc_mx_request_from_down(vc_mx_t *vc_mx);

#endif /* SDL_USDL_VC_MULTIPLEX_H_ */
