/*
 * ccscds_endian.h
 *
 *  Created on: 18 џэт. 2021 у.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_CCSCDS_ENDIAN_H_
#define INCLUDE_CCSDS_CCSCDS_ENDIAN_H_

#include <endian.h>

void ccsds_endian_set(uint8_t *arr, int arr_bit_size, uint8_t *field,
		int field_bit_size, int arr_bit_pos) {
	endian_set(arr, arr_bit_size, field,
			field_bit_size, arr_bit_pos, endian_host(), ENDIAN_MSBIT_LSBYTE);
}

#endif /* INCLUDE_CCSDS_CCSCDS_ENDIAN_H_ */
