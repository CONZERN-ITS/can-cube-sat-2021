/*
 * epp.h
 *
 *  Created on: 17 янв. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_NL_EPP_EPP_H_
#define INCLUDE_CCSDS_NL_EPP_EPP_H_

#include <inttypes.h>

typedef enum {
	EPP_ID_IDLE = 0x0,
	EPP_ID_EXTENSION = 0x6,
} epp_id_t;

typedef struct {
	uint8_t pvn;
	epp_id_t epp_id;
	uint8_t lol;
	uint8_t udf;
	uint8_t epp_id_ex;
	uint16_t ccsds_defined_field;
	uint32_t packet_length;
} epp_header_t;

int epp_make_header(epp_header_t *epp_header, uint8_t *arr, int size);

void epp_extract_header(epp_header_t *epp_header, const uint8_t *arr, int size);

int epp_make_header_auto_length(epp_header_t *epp_header, uint8_t *arr, int size, uint32_t packet_length);

#endif /* INCLUDE_CCSDS_NL_EPP_EPP_H_ */
