/*
 * ccscds_endian.h
 *
 *  Created on: 18 янв. 2021 г.
 *      Author: HP
 */

#ifndef INCLUDE_CCSDS_CCSCDS_ENDIAN_H_
#define INCLUDE_CCSDS_CCSCDS_ENDIAN_H_

/*
 * Вставаляет в массив поле field
 * Для правильной работы в field передайте
 * адрес переменной, который вы хотите вставить в массив
 * байт arr.
 */
void ccsds_endian_insert(uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		const void *field, int field_bit_size);

/*
 * Достает из массива поле field
 * Для правильной работы в field передайте
 * адрес переменной, в которыую вы хотите записать
 * значение из массив байт arr.
 */
void ccsds_endian_extract(const uint8_t *arr, int arr_bit_size, int arr_bit_pos,
		void *field, int field_bit_size);

#endif /* INCLUDE_CCSDS_CCSCDS_ENDIAN_H_ */
