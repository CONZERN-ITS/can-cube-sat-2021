/*
 * ccsds_endian.c
 *
 *  Created on: 18 янв. 2021 г.
 *      Author: HP
 */


#include <endian.h>

void ccsds_endian_insert(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		const void *field, int field_bit_size) {
	endian_stream_set(arr, arr_bit_size, arr_bit_pos, ENDIAN_MSBIT_LSBYTE,
			field, field_bit_size, 0, endian_host());
}

void ccsds_endian_extract(const uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		void *field, int field_bit_size) {
	endian_stream_set(field, field_bit_size, 0, endian_host(),
			arr, arr_bit_size, arr_bit_pos, ENDIAN_MSBIT_LSBYTE);
}
