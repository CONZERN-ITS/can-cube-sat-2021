/*
 * mapa.h
 *
 *  Created on: 27 янв. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_SDL_USDL_MAPA_SERVICE_H_
#define INCLUDE_CCSDS_SDL_USDL_MAPA_SERVICE_H_

#include <ccsds/sdl/usdl/map_service_abstract.h>


int mapa_init(map_t *map, vc_t *vc, map_id_t map_id);

int mapa_send(map_t *map, const uint8_t *data, size_t size, quality_of_service_t qos);

int mapa_recieve(mapr_t *map, uint8_t *data, size_t size, quality_of_service_t *qos);

#endif /* INCLUDE_CCSDS_SDL_USDL_MAPA_SERVICE_H_ */
