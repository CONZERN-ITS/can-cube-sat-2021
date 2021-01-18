/*
 * ccscds_endian.h
 *
 *  Created on: 18 ���. 2021 �.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_CCSCDS_ENDIAN_H_
#define INCLUDE_CCSDS_CCSCDS_ENDIAN_H_

/*
 * ���������� � ������ ���� field
 * ��� ���������� ������ � field ��������� ���������������
 * � uint8_t* ����� ����������, ������� �� ������ �������� � ������
 * ���� arr.
 */
void ccsds_endian_set(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		uint8_t *field,	int field_bit_size);

/*
 * ������� �� ������� ���� field
 * ��� ���������� ������ � field ��������� ���������������
 * � uint8_t* ����� ����������, � �������� �� ������ ��������
 * �������� �� ������ ���� arr.
 */
void ccsds_endian_get(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		uint8_t *field,	int field_bit_size);

#endif /* INCLUDE_CCSDS_CCSCDS_ENDIAN_H_ */
