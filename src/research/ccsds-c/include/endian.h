/*
 * endian.h
 *
 *  Created on: 15 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_ENDIAN_H_
#define SDL_ENDIAN_H_

typedef enum {
	ENDIAN_LSBIT_LSBYTE,
	ENDIAN_LSBIT_MSBYTE,
	ENDIAN_MSBIT_LSBYTE,
	ENDIAN_MSBIT_MSBYTE,
} endian_t;


void print_bit(uint8_t num) {
	for(int i = 0; i < 8; ++i) {
		printf("%u", num >> (7 - i) & 1);
	}
}

void endian_set(uint8_t *arr, int arr_bit_size, uint8_t *field,
		int field_bit_size, int arr_bit_pos, endian_t from, endian_t to);

endian_t endian_host();


#endif /* SDL_ENDIAN_H_ */
