/*
 * p_generate.h
 *
 *  Created on: 16 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_USDL_PC_GENERATE_H_
#define SDL_USDL_PC_GENERATE_H_

#include "usdl_types.h"
#include "vc_multiplex.h"


int pc_generate(pc_t *pc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params);

#endif /* SDL_USDL_PC_GENERATE_H_ */
