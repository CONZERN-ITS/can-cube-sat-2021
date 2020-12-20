#include "sx126x_defs.h"

#include <stdlib.h>  // abs()


const sx126x_pa_coeffs_t sx126x_pa_coeffs_1261[] = {
		{ 10, 0x01, 0x00, 0x01, 0x01, 13, 0x18 },
		{ 14, 0x04, 0x00, 0x01, 0x01, 14, 0x18 },
		{ 15, 0x06, 0x00, 0x01, 0x01, 14, 0x18 },
};


const sx126x_pa_coeffs_t sx126x_pa_coeffs_1262[] = {
		{ 14, 0x02, 0x02, 0x00, 0x01, 22, 0x38 },
		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 },
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 },
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 },
};


const sx126x_pa_coeffs_t sx126x_pa_coeffs_1268[] = {
		// Эти два режима оптимальны для питания чипа от DC-DC ?
		{ 10, 0x00, 0x03, 0x00, 0x01, 15, 0x18 },
		{ 14, 0x04, 0x06, 0x00, 0x01, 15, 0x18 },

		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 },
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 },
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 },
};


const sx126x_pa_coeffs_t * sx126x_defs_get_pa_coeffs(int8_t power, sx126x_chip_type_t pa_type)
{
	const sx126x_pa_coeffs_t * coeffs = NULL;
	size_t coeffs_count = 0;

	switch (pa_type)
	{
	case SX126X_CHIPTYPE_SX1261:
		coeffs = sx126x_pa_coeffs_1261;
		coeffs_count = sizeof(sx126x_pa_coeffs_1261)/sizeof(sx126x_pa_coeffs_1261[0]);
		break;

	case SX126X_CHIPTYPE_SX1262:
		coeffs = sx126x_pa_coeffs_1262;
		coeffs_count = sizeof(sx126x_pa_coeffs_1262)/sizeof(sx126x_pa_coeffs_1262[0]);
		break;

	case SX126X_CHIPTYPE_SX1268:
		coeffs = sx126x_pa_coeffs_1268;
		coeffs_count = sizeof(sx126x_pa_coeffs_1268)/sizeof(sx126x_pa_coeffs_1268[0]);
		break;

	default:
		return NULL;
	};


	// Ищем ближайшее число к указанному в массиве данные коэффициентов
	size_t best_idx = 0;
	int16_t best_diff = abs(power - coeffs[0].power);

	for (size_t i = 1; i < coeffs_count; i++)
	{
		uint16_t diff = abs(power - coeffs[i].power);
		if (diff < best_diff)
		{
			best_diff = diff;
			best_idx = i;
		}
	}

	return coeffs + best_idx;
}
