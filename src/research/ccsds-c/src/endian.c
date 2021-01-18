#include <endian.h>


/*
 * ������� ����� � �������� ���������� ��������� �� ���� 0*1*0*
 */
#define MASK(type,l,r) (type)((type)((type)(~0) >> (8 * sizeof(type) - (r - l))) << l)
#define MASK_U(l,r) MASK(uint8_t,l,r)

/*
 * �������� ������� ����� �� from � *to
 * l, r ������ ������� ����������� �� from.
 * to_pos - ��������� ������� ������ ���.
 * to_l, to_r ������ ���������� ������� ������ ��� � *to.
 */
int bit_set(uint8_t *to, uint8_t from, int to_l, int to_r, int to_pos, int l, int r) {
	//�� �� ����� ������ �������� ��� ������ ������ �������
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
 * ������� ���� �� ������� ��� � ������ ������� ��� � ���� � arr
 */
uint8_t get_from_arr_en(uint8_t *arr, endian_t en, int bit_size, int bit_pos) {
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
	 * ���� t ������������ �� ���� ������, ������� ����������� � ����
	 * ��������������� ������ �������.
	 *
	 * ��� �������� ���������� bit_set ���������� � ������� ������� �
	 * ������� �����������. ���� ���-�� �������� �� ������������,
	 * ����� ��������� �����������. - SeresHotes
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
 * ��������� val � ������ ��� � ������ ������� ��� � ���� � arr
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
	 * ���� val ����������� �� ��� ������, ������� ����� ����������� � ����
	 * ��������������� ������ �������.
	 *
	 * ��� �������� ���������� bit_set ���������� � ������� ������� �
	 * ������� �����������. ���� ���-�� �������� �� ������������,
	 * ����� ��������� �����������. - SeresHotes
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
 * �������� �� ������ ������� ��� � ������ ������ ��� � ������
 * ������� ��� � ���� ����� ��������
 */
void endian_set(uint8_t *to, int to_bit_size, int to_bit_pos, endian_t to_endian,
		uint8_t *from, int from_bit_size, int from_bit_pos, endian_t from_endian) {
	/*
	 * ������� ������ ���� ������, ���� ������� ��� ���������
	 * TODO: ���� 50% ����������� ����, ��� ��� �������. �� ���
	 * ����� �����, ���� ���� - LSBit/LSByte.
	 */
	if (to_endian / 2 == from_endian / 2) {
		for (int i = 0; i < (from_bit_size + 7) / 8; i++) {
			uint8_t t = get_from_arr_en(from, from_endian, from_bit_size, from_bit_pos + 8 * i);
			set_to_arr_en(to, to_endian, to_bit_size, to_bit_pos + 8 * i, t);
		}
	} else {
		for (int i = 0; i < (from_bit_size + 7) / 8; i++) {
			uint8_t t = get_from_arr_en(from, from_endian, from_bit_size, from_bit_pos - 8 - 8 * i);
			set_to_arr_en(to, to_endian, to_bit_size, to_bit_pos + 8 * i, t);
		}
	}
}

/*
 * ���������� ������� ��� � ����� �����
 */
endian_t endian_host() {
	uint16_t t = 1;
	uint8_t *a = (uint8_t *)&t;
	return a[0] == t ? ENDIAN_LSBIT_LSBYTE : ENDIAN_LSBIT_MSBYTE;
}

