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


int mc_generate(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);


#endif /* SDL_USDL_MC_GENERATE_H_ */
