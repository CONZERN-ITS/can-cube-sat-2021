/*
 * fop.h
 *
 *  Created on: 14 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_FOP_H_
#define SDL_USDL_FOP_H_


#include "usdl_types.h"

#include <string.h>
#include <assert.h>



int queue_push(queue_t *queue, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);

void queue_drop_first(queue_t *queue);

void queue_drop_all(queue_t *queue);

int queue_drop(queue_t *queue, vc_id_t vc_id);

int queue_pop(queue_t *queue, uint8_t *data, size_t *size,
		map_params_t *map_params, vc_params_t *vc_params);

queue_element_t *queue_peek(queue_t *queue);

int queue_is_full(queue_t *queue);


int fop_add_packet(fop_t *fop, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params);

queue_value_t *fop_peek(fop_t *fop);

void fop_drop(fop_t *fop);

int fop_control();

#endif /* SDL_USDL_FOP_H_ */
