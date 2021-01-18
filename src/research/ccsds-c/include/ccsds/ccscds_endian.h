/*
 * ccscds_endian.h
 *
 *  Created on: 18 €нв. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_CCSCDS_ENDIAN_H_
#define INCLUDE_CCSDS_CCSCDS_ENDIAN_H_

/*
 * ¬ставал€ет в массив поле field
 * ƒл€ правильной работы в field передайте преобразованный
 * к uint8_t* адрес переменной, который вы хотите вставить в массив
 * байт arr.
 */
void ccsds_endian_set(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		uint8_t *field,	int field_bit_size);

/*
 * ƒостает из массива поле field
 * ƒл€ правильной работы в field передайте преобразованный
 * к uint8_t* адрес переменной, в которыую вы хотите записать
 * значение из массив байт arr.
 */
void ccsds_endian_get(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		uint8_t *field,	int field_bit_size);

#endif /* INCLUDE_CCSDS_CCSCDS_ENDIAN_H_ */
