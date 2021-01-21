/*
 * p_generate.h
 *
 *  Created on: 16 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_PC_GENERATE_H_
#define SDL_USDL_PC_GENERATE_H_

#include "usdl_types.h"
#include "vc_multiplex.h"


int pc_init(pc_t *pc, const pc_paramaters_t *params, const mc_mx_t *mc_mx, const uint8_t *data, size_t size);

int pc_generate(pc_t *pc, const uint8_t *data, size_t size,
		const map_params_t *map_params, const vc_params_t *vc_params, const mc_params_t *mc_params);


#endif /* SDL_USDL_PC_GENERATE_H_ */
