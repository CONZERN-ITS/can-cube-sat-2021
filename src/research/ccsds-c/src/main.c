/*
 * main.c
 *
 *  Created on: 15 џэт. 2021 у.
 *      Author: HP
 */

#include <ccsds/sdl/usdl/usdl.h>
#include <stdio.h>
#include <endian.h>

void print_bit(uint8_t num) {
	for(int i = 0; i < 8; ++i) {
		printf("%u", num >> (7 - i) & 1);
	}
}


void print_bit_arr(uint8_t *arr, size_t size) {
	for (int i = 0; i < size; i++) {
		print_bit(arr[i]);
		printf(" ");
	}
}

int main() {
	uint8_t a = 0b10101010;
	a = 0b11111111;
	const int size = 4;
	uint8_t arr[size];
	memset(arr, 0, size);

	//print_bit(bit_set(0, a, 8, 0, 4, 8));
	uint8_t arr2[3] = {0b10010010, 0b00001101, 0b00101011};
	uint16_t value = 0b1001110110100011;
	endian_set(arr, 28, 14, ENDIAN_MSBIT_LSBYTE,
			(uint8_t *)&value, sizeof(value) * 8, 0, endian_host());
	print_bit_arr(arr, size);
	//set_to_arr_en(arr, 3, 21, 15, a);
	//print_bit_arr(arr, size);

	//endian(arr, 21, &a, 8, 8, 0, 2);
	//print_bit_arr(arr, size);
	uint8_t b;
	a = 0b00000000;
	b = 0b11110011;
	//print_bit(a);
	//print_bit(set(a, b, 2, 4, 5));
	return 0;
}



