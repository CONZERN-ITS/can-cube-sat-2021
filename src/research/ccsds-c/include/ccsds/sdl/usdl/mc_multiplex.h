/*
 * mc_multiplex.h
 *
 *  Created on: 16 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_MC_MULTIPLEX_H_
#define SDL_USDL_MC_MULTIPLEX_H_

#include "usdl_types.h"
#include "pc_generate.h"

int mc_mx_init(mc_mx_t *mc_mx, pc_t *pc);

int mc_multiplex(mc_mx_t *mc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params);

int mc_mx_request_from_down(mc_mx_t *mc_mx);

void mc_mx_demultiplex(mc_mx_t *mc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params, uint8_t *ocf);

#endif /* SDL_USDL_MC_MULTIPLEX_H_ */
