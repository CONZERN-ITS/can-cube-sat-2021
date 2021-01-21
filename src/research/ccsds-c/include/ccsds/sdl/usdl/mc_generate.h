/*
 * mc_generate.h
 *
 *  Created on: 15 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_MC_GENERATE_H_
#define SDL_USDL_MC_GENERATE_H_

#include "usdl_types.h"
#include "mc_multiplex.h"


int mc_init(mc_t *mc, mc_mx_t *mx, mc_paramaters_t *params, mc_id_t mc_id);

int mc_generate(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);


int mc_request_from_down(mc_t *mc);

uint32_t _mc_request_size(mc_t *mc);

#endif /* SDL_USDL_MC_GENERATE_H_ */
