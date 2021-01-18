#ifndef SDL_ENDIAN_H_
#define SDL_ENDIAN_H_

#include <inttypes.h>

typedef enum {
	ENDIAN_LSBIT_LSBYTE,
	ENDIAN_LSBIT_MSBYTE,
	ENDIAN_MSBIT_LSBYTE,
	ENDIAN_MSBIT_MSBYTE,
} endian_t;


/*
 * �������� from_bit_size ���, ������� � ������� ���� from_bit_pos
 * �� ������� from � �������� ��� � ���� from_endian � ������ to ��������
 * to_bit_size ������� � ������� to_bit_pos � �������� ��� � ���� from_endian
 *
 * ����� ������ ��� ������������ � �������������� ����� � ��������� ������
 * ����������.
 */
void endian_set(uint8_t *to, int to_bit_size, int to_bit_pos, endian_t to_endian,
		uint8_t *from, int from_bit_size, int from_bit_pos, endian_t from_endian);

/*
 * ���������� ������� ��� � ���� �����
 */
endian_t endian_host();


#endif /* SDL_ENDIAN_H_ */
