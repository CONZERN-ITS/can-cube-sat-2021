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
#include <string.h>
#include <assert.h>

int _vc_fop();

int _vc_pop_fop(vc_t *vc);

int vc_init(vc_t *vc, mc_t *mc, vc_parameters_t *params, vc_id_t vc_id);

int vc_generate(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size);

int vc_push(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size);

int vc_request_from_down(vc_t *vc);

uint32_t _vc_request_size(vc_t *vc, quality_of_service_t qos);

void vc_parse(vc_t *vc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);

#endif /* SDL_USDL_VC_GENERATE_H_ */
