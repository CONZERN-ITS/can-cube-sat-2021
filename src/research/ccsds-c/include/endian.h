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
 * Копирует from_bit_size бит, начиная с позиции бита from_bit_pos
 * из массива from с порядком бит и байт from_endian в массив to размером
 * to_bit_size начиная с позиции to_bit_pos и порядком бит и байт from_endian
 *
 * Очень удобна для сериализации и десериализации полей в заголвках разных
 * протоколов.
 */
void endian_set(uint8_t *to, int to_bit_size, int to_bit_pos, endian_t to_endian,
		uint8_t *from, int from_bit_size, int from_bit_pos, endian_t from_endian);

/*
 * Возвращает порядок бит и байт хоста
 */
endian_t endian_host();


#endif /* SDL_ENDIAN_H_ */
