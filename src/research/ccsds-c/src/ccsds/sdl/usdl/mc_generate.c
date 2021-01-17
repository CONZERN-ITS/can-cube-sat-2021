/*
 * mc_generate.h
 *
 *  Created on: 15 ���. 2021 �.
 *      Author: HP
 */

#ifndef SDL_USDL_MC_GENERATE_H_
#define SDL_USDL_MC_GENERATE_H_

#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/vc_multiplex.h>
#include <ccsds/sdl/usdl/mc_multiplex.h>
#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>


int mc_generate(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {

	mc_params_t p = {0};
	return mc_multiplex(mc->mc_mx, data, size, map_params, vc_params, &p);
}


#endif /* SDL_USDL_MC_GENERATE_H_ */