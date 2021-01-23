/*
 * usdl_debug.h
 *
 *  Created on: 22 янв. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_SDL_USDL_USDL_DEBUG_H_
#define INCLUDE_CCSDS_SDL_USDL_USDL_DEBUG_H_

#include <ccsds/sdl/usdl/usdl_defines.h>

#ifdef USDL_DEBUG
#include <stdio.h>
#define usdl_print_debug() printf("%s: %d: %s\n", __FILE__, __LINE__, __FUNCTION__); fflush(stdout)
#else
#define usdl_print_debug()
#endif

#endif /* INCLUDE_CCSDS_SDL_USDL_USDL_DEBUG_H_ */
