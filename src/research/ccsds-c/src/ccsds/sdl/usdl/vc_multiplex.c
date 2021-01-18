/*
 * vc_multiplex.h
 *
 *  Created on: 15 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_MULTIPLEX_H_
#define SDL_USDL_VC_MULTIPLEX_H_

#include <ccsds/sdl/usdl/vc_multiplex.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/usdl_types.h>

int vc_multiplex(vc_mx_t *vc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	if (vc_params->vc_id != vc_mx->vc_active) {
		return 0;
	} else {
		int ret = mc_generate(vc_mx->mc, data, size, map_params, vc_params);
		if (ret) {
			mc_id_t old = vc_mx->vc_active;
			do {
				vc_mx->vc_active++;
			} while (vc_mx->vc_active != old && vc_mx->vc_arr[vc_mx->vc_active] == 0);
		}
		return ret;
	}
}


#endif /* SDL_USDL_VC_MULTIPLEX_H_ */
