/*
 * vc_multiplex.h
 *
 *  Created on: 15 ���. 2021 �.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_MULTIPLEX_H_
#define SDL_USDL_VC_MULTIPLEX_H_

#include "usdl_types.h"
#include "mc_generate.h"

int vc_multiplex(vc_mx_t *vc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);


#endif /* SDL_USDL_VC_MULTIPLEX_H_ */
