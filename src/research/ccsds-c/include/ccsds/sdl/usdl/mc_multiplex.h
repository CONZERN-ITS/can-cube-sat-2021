/*
 * mc_multiplex.h
 *
 *  Created on: 16 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_USDL_MC_MULTIPLEX_H_
#define SDL_USDL_MC_MULTIPLEX_H_

#include "usdl_types.h"
#include "pc_generate.h"

int mc_multiplex(mc_mx_t *mc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params);


#endif /* SDL_USDL_MC_MULTIPLEX_H_ */
