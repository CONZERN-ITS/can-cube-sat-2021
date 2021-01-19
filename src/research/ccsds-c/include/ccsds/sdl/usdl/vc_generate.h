/*
 * vc_generation.h
 *
 *  Created on: 9 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_GENERATE_H_
#define SDL_USDL_VC_GENERATE_H_

#include "usdl_types.h"
#include "fop.h"
#include "vc_multiplex.h"
#include <string.h>
#include <assert.h>

int _vc_fop();

int _vc_pop_fop(vc_t *vc);

int vc_generate(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size);

int vc_request_from_down(vc_t *vc);



#endif /* SDL_USDL_VC_GENERATE_H_ */
