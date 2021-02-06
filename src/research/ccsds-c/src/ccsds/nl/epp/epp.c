/*
 * epp.c
 *
 *  Created on: 19 янв. 2021 г.
 *      Author: HP
 */


#include <ccsds/nl/epp/epp.h>
#include <ccsds/ccscds_endian.h>

const uint8_t epp_pvn = 0x7;

int epp_make_header(epp_header_t *epp_header, uint8_t *arr, int size) {
	epp_header->pvn = epp_pvn;

	if (1 << epp_header->lol > size ) {
		return 0;
	}
	int k = 0;
	ccsds_endian_insert(arr, size * 8, k, &epp_header->pvn, 3);
	ccsds_endian_insert(arr, size * 8, k + 3, &epp_header->epp_id, 3);
	ccsds_endian_insert(arr, size * 8, k + 6, &epp_header->lol, 2);
	k += 8;
	if (epp_header->lol > 2) {
		ccsds_endian_insert(arr, size * 8, k, &epp_header->udf, 4);
		ccsds_endian_insert(arr, size * 8, k + 4, &epp_header->epp_id_ex, 4);
		k += 8;
	}
	if (epp_header->lol == 3) {
		ccsds_endian_insert(arr, size * 8, k, &epp_header->ccsds_defined_field, 16);
		k += 16;
	}
	if (epp_header->lol > 0) {
		ccsds_endian_insert(arr, size * 8, k, &epp_header->packet_length, 1 << (epp_header->lol + 2));
		k += 1 << (epp_header->lol + 2);
	}
	return k / 8;
}

int epp_make_header_auto_length(epp_header_t *epp_header, uint8_t *arr, int size, uint32_t packet_length) {
	epp_header->pvn = epp_pvn;

	epp_header->lol = 0;
	if (packet_length == 1) {
		epp_header->lol = 0;
	} else if (packet_length < 1 << 8) {
		epp_header->lol = 1;
	} else if (packet_length < 1 << 16) {
		epp_header->lol = 2;
	} else {
		epp_header->lol = 3;
	}
	epp_header->packet_length = packet_length;

	return epp_make_header(epp_header, arr, size);
}
int epp_make_header_auto_length2(epp_header_t *epp_header, uint8_t *arr, int size, uint32_t payload_length) {
	epp_header->pvn = epp_pvn;

	epp_header->lol = 0;
	if (payload_length == 0) {
		epp_header->lol = 0;
	} else if (payload_length < (1 << 8) - 1) {
		epp_header->lol = 1;
	} else if (payload_length < (1 << 16) - 2) {
		epp_header->lol = 2;
	} else {
		epp_header->lol = 3;
	}
	epp_header->packet_length = payload_length + (1 << epp_header->lol);

	return epp_make_header(epp_header, arr, size);
}


int epp_extract_header(epp_header_t *epp_header, const uint8_t *arr, int size) {
	if (size <= 0) {
		return 0;
	}
	int k = 0;
	ccsds_endian_extract(arr, k, &epp_header->pvn, 3);
	ccsds_endian_extract(arr, k + 3, &epp_header->epp_id, 3);
	ccsds_endian_extract(arr, k + 6, &epp_header->lol, 2);
	k += 8;
	if (1 << epp_header->lol > size) {
		return 0;
	}

	if (epp_header->lol >= 2) {
		ccsds_endian_extract(arr, k, &epp_header->udf, 4);
		ccsds_endian_extract(arr, k + 4, &epp_header->epp_id_ex, 4);
		k += 8;
	}
	if (epp_header->lol == 3) {
		ccsds_endian_extract(arr, k, &epp_header->ccsds_defined_field, 16);
		k += 16;
	}
	if (epp_header->lol > 0) {
		ccsds_endian_extract(arr, k, &epp_header->packet_length, 1 << (epp_header->lol + 2));
		k += 1 << (epp_header->lol + 2);
	}
	return 1 << epp_header->lol;
}

