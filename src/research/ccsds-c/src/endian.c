#include <endian.h>


/*
 * Создает маску в терминах регулярных выржениях по типу 0*1*0*
 */
#define MASK(type,l,r) (type)((type)((type)(~0) >> (8 * sizeof(type) - (r - l))) << l)
#define MASK_U(l,r) MASK(uint8_t,l,r)

/*
 * Копирует отрезок битов из from в *to
 * l, r задают границы копирования из from.
 * to_pos - стартовая позиция записи бит.
 * to_l, to_r задают допустимые границы записи бит в *to.
 */
int bit_set(uint8_t *to, uint8_t from, int to_l, int to_r, int to_pos, int l, int r) {
	//Мы не хотим делать проверки при каждом вызове функции
	if (to_l >= 8 || to_r <= 0) {
		return -1;
	}
	if (to_l <= 0) {
		to_l = 0;
	}
	if (to_r > 8) {
		to_r = 8;
	}
	*to = *to & (~MASK_U(to_pos,(to_pos + r - l)) | ~MASK_U(to_l, to_r));
	from = (from & MASK_U(l,r)) >> l;
	from = from << to_pos;
	*to = *to | (from & MASK_U(to_l, to_r));
	return 0;
}

/*
 * Достает байт из массива бит с учетом порядка бит и байт в arr
 */
uint8_t get_from_arr_en(const uint8_t *arr, endian_t en, int bit_size, int bit_pos) {
	int shift = bit_pos % 8;
	uint8_t t = 0;

	int flag_bit = en / 2;
	int flag_byte = en % 2;
	int i1 = 0;
	int i2 = 0;
	if (!flag_byte) {
		i1 = bit_pos / 8;
		i2 = bit_pos / 8 + 1;
	} else {
		i1 = (bit_size - 1) / 8 - bit_pos / 8;
		i2 = i1 - 1;
	}

	/*
	 * Байт t составляется из двух кусков, которые расположены в двух
	 * последовтельных байтах массива.
	 *
	 * Все значения параметров bit_set определены с помощью чистого и
	 * долгого размышления. Если кто-то проверит их правильность,
	 * прошу дополнить комментарий. - SeresHotes
	 */
	if (!flag_byte) {
		if (!flag_bit) {
			bit_set(&t, arr[i1],  0, bit_size - i1 * 8, 0, shift, 8);
			bit_set(&t, arr[i2],  0, bit_size - i2 * 8, 8 - shift, 0, shift);

		} else {
			bit_set(&t, arr[i1], (i1 + 1) * 8 - bit_size, 8, shift, 0, 8 - shift);
			bit_set(&t, arr[i2], (i2 + 1) * 8 - bit_size, 8, 0, 8 - shift, 8);
		}
	} else {
		if (!flag_bit) {
			bit_set(&t, arr[i1], 0, bit_size % 8 + i1 * 8, 0, shift, 8);
			bit_set(&t, arr[i2], 0, bit_size % 8 + i2 * 8, 8 - shift, 0, shift);

		} else {
			bit_set(&t, arr[i1], (1 - i1) * 8 - (bit_size % 8), 8, shift, 0, 8 - shift);
			bit_set(&t, arr[i2], (1 - i2) * 8 - (bit_size % 8), 8, 0, 8 - shift, 8);
		}
	}
	return t;
}

/*
 * Запиывает val в массив бит с учетом порядка бит и байт в arr
 */
void set_to_arr_en(uint8_t *arr, endian_t en, int bit_size, int bit_pos, uint8_t val) {

	int shift = bit_pos % 8;

	int flag_bit = en / 2;
	int flag_byte = en % 2;
	int i1 = 0;
	int i2 = 0;
	if (!flag_byte) {
		i1 = bit_pos / 8;
		i2 = bit_pos / 8 + 1;
	} else {
		i1 = (bit_size - 1) / 8 - bit_pos / 8;
		i2 = i1 - 1;
	}
	/*
	 * Байт val разбивается на два кусков, которые будут расположены в двух
	 * последовтельных байтах массива.
	 *
	 * Все значения параметров bit_set определены с помощью чистого и
	 * долгого размышления. Если кто-то проверит их правильность,
	 * прошу дополнить комментарий. - SeresHotes
	 */
	if (!flag_byte) {
		if (!flag_bit) {
			bit_set(&arr[i1], val, 0, bit_size - i1 * 8, shift, 0, 8 - shift);
			bit_set(&arr[i2], val, 0, bit_size - i2 * 8, 0, 8 - shift, 8);

		} else {
			bit_set(&arr[i1], val, (i1 + 1) * 8 - bit_size, 8, 0, shift, 8);
			bit_set(&arr[i2], val, (i2 + 1) * 8 - bit_size, 8, 8 - shift, 0, shift);
		}
	} else {
		if (!flag_bit) {
			bit_set(&arr[i1], val, 0, bit_size % 8 + i1 * 8, shift, 0, 8 - shift);
			bit_set(&arr[i2], val, 0, bit_size % 8 + i2 * 8, 0, 8 - shift, 8);

		} else {
			bit_set(&arr[i1], val, (1 - i1) * 8 - (bit_size % 8), 8, 0, shift, 8);
			bit_set(&arr[i2], val, (1 - i2) * 8 - (bit_size % 8), 8, 8 - shift, 0, shift);
		}
	}
}

/*
 * Копирует из одного массива бит в другой массив бит с учетом
 * порядка бит и байт обоих массивов
 */
void endian_set(uint8_t *to, int to_bit_size, int to_bit_pos, endian_t to_endian,
		const uint8_t *from, int from_bit_size, int from_bit_pos, endian_t from_endian) {
	/*
	 * Порядок записи байт прямой, если порядок бит совпадает
	 * TODO: есть 50% вероятность того, что это неверно. Но это
	 * точно верно, если хост - LSBit/LSByte.
	 */
	if (to_endian / 2 == from_endian / 2) {
		for (int i = 0; i < (from_bit_size + 7) / 8; i++) {
			uint8_t t = get_from_arr_en(from, from_endian, from_bit_size, from_bit_pos + 8 * i);
			set_to_arr_en(to, to_endian, to_bit_size, to_bit_pos + 8 * i, t);
		}
	} else {
		for (int i = 0; i < (from_bit_size + 7) / 8; i++) {
			uint8_t t = get_from_arr_en(from, from_endian, from_bit_size, from_bit_size - 8 - 8 * i);
			set_to_arr_en(to, to_endian, to_bit_size, to_bit_pos + 8 * i, t);
		}
	}
}

/*
 * Возвращает порядок бит и байта хоста
 */
endian_t endian_host() {
	uint16_t t = 1;
	uint8_t *a = (uint8_t *)&t;
	return a[0] == t ? ENDIAN_LSBIT_LSBYTE : ENDIAN_LSBIT_MSBYTE;
}

uint8_t _stream_get_bit(const uint8_t *from, int bit_size, int bit_pos, endian_t en) {
	if (bit_pos >= bit_size) {
		return 0;
	}
	int shift = bit_pos % 8;
	int index = bit_pos / 8;
	int size = (bit_size + 7) / 8;
	int bit_flag = en / 2;
	int byte_flag = en % 2;

	shift = (-2 * shift + 7) * bit_flag + shift;
	index = (-2 * index + size - 1) * byte_flag + index;

	return (from[index] >> shift) & 1;
}

void _stream_set_bit(uint8_t *to, int bit_size, int bit_pos, endian_t en, uint8_t bit) {
	if (bit_pos >= bit_size) {
		return;
	}
	int shift = bit_pos % 8;
	int index = bit_pos / 8;
	int size = (bit_size + 7) / 8;
	int bit_flag = en / 2;
	int byte_flag = en % 2;

	shift = (-2 * shift + 7) * bit_flag + shift;
	index = (-2 * index + size - 1) * byte_flag + index;

	to[index] &= ~(1 << shift);
	to[index] |= bit << shift;

}

void endian_stream_set(uint8_t *to, int to_bit_size, int to_bit_pos, endian_t to_endian,
		const uint8_t *from, int from_bit_size, int from_bit_pos, endian_t from_endian) {
	if (to_endian / 2 == from_endian / 2) {
		for (int i = 0; i < from_bit_size - from_bit_pos
					 && i < to_bit_size - to_bit_pos; i++) {
			int t = _stream_get_bit(from, from_bit_size, from_bit_pos + i, from_endian);
			_stream_set_bit(to, to_bit_size, to_bit_pos + i, to_endian, t);
		}
	} else {
		for (int i = 0; i < from_bit_size - from_bit_pos
					 && i < to_bit_size - to_bit_pos; i++) {
			int t = _stream_get_bit(from, from_bit_size, from_bit_pos + i, from_endian);
			_stream_set_bit(to, to_bit_size, to_bit_pos + (from_bit_size - from_bit_pos - 1 - i), to_endian, t);
		}
	}
}
