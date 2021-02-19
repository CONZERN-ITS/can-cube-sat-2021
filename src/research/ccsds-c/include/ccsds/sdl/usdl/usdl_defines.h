/*
 * usdl_defines.h
 *
 *  Created on: 8 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_USDL_DEFINES_H_
#define SDL_USDL_USDL_DEFINES_H_

#define USDL_MAP_MAX_COUNT 6
#define USDL_VC_MAX_COUNT 6
#define USDL_MC_MAX_COUNT 1
#define USDL_PC_MAX_COUNT 1
#define USDL_MX_MAX_COUNT (USDL_VC_MAX_COUNT + USDL_MC_MAX_COUNT + USDL_PC_MAX_COUNT)

#define MAP_BUFFER_STATIC
#ifdef MAP_BUFFER_STATIC
#define MAP_BUFFER_SIZE 100
#endif

#define MAP_COUNT 16
#define VC_COUNT 64
#define MC_COUNT 1
#define USDL_DEBUG


#endif /* SDL_USDL_USDL_DEFINES_H_ */
