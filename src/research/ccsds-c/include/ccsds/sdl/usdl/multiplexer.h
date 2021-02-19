/*
 * multiplexe.h
 *
 *  Created on: 16 февр. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_SDL_USDL_MULTIPLEXER_H_
#define INCLUDE_CCSDS_SDL_USDL_MULTIPLEXER_H_

#include <assert.h>
#include <ccsds/sdl/usdl/usdl_basic_types.h>

struct multiplexer_abstract {
	int (*push)(struct multiplexer_abstract *, usdl_node_t *p, usdl_node_t **arr, int size);
	int (*pull)(struct multiplexer_abstract *, usdl_node_t **arr, int size);
};

struct multiplexer_rr {
	struct multiplexer_abstract base;
	int prev;
};

void mx_construct_multiplexer_rr(struct multiplexer_rr *mx);

#endif /* INCLUDE_CCSDS_SDL_USDL_MULTIPLEXER_H_ */
