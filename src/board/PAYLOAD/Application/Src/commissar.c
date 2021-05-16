#include "commissar.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include <stm32f4xx_hal.h>

#include "sensors/its_bme280.h"
#include "sensors/its_ms5611.h"


//! Личное дело подчиненного комиссара
typedef struct subordinate_file_t
{
	//! Время последнего хорошего отчета
	uint32_t last_good_report_time;

	//! Общее количество отчетов подчиненного за всю карьеру комиссара
	uint32_t reports_counter;
	//! Общее количество ошибок подчиенного за всю карьеру комиссара.
	uint32_t mistakes_counter;

	//! Код последней ошибки
	int last_mistake;
	//! Длина серии последовательных ошибок
	uint32_t ignorance_depth;
	//! Время последнего плохого очета
	uint32_t last_wrong_report_time;

	//! Количество попыток поднятия модуля
	uint32_t punishments_counter;
	//! Метка времени последней попытки поднятия модуля
	uint32_t last_pusnishment_time;

} subordinate_file_t;


typedef struct commissar_t
{
	subordinate_file_t subors[COMMISSAR__SUBS_COUNT];
} commissar_t;


static commissar_t commissar;


static subordinate_file_t * _get_subor(commissar_t * const self, commissar_subordinate_t who)
{
	assert((int)who >= 0 && (int)who < COMMISSAR__SUBS_COUNT);
	return &self->subors[who];
}


static int _punish_the_heretic(commissar_t * const self, commissar_subordinate_t who)
{
	int rc;
	subordinate_file_t * subor = _get_subor(self, who);

	switch (who)
	{
	case COMMISSAR_SUB_BME280_INT:
		rc =its_bme280_punish(ITS_BME_LOCATION_INTERNAL);
		break;

	case COMMISSAR_SUB_MS5611_INT:
		rc = its_ms5611_punish(ITS_MS_INTERNAL);
		break;

	case COMMISSAR_SUB_BME280_EXT:
		rc = its_bme280_punish(ITS_BME_LOCATION_EXTERNAL);
		break;

	case COMMISSAR_SUB_MS5611_EXT:
		rc = its_ms5611_punish(ITS_MS_EXTERNAL);
		break;

	default:
		abort();
	}

	subor->punishments_counter++;
	subor->last_pusnishment_time = HAL_GetTick();
	return rc;
}


void commissar_init(void)
{
	commissar_t * const self = &commissar;

	const uint32_t now = HAL_GetTick();
	for (size_t i = 0; i < COMMISSAR__SUBS_COUNT; i++)
	{
		subordinate_file_t * const subor = _get_subor(self, i);
		memset(subor, 0x00, sizeof(*subor));
		// Будем полагать что по-умолчанию все хорошие
		subor->last_good_report_time = now;
	}
}


void commissar_report(commissar_subordinate_t who, int error_code)
{
	commissar_t * const self = &commissar;
	subordinate_file_t * const subor = _get_subor(self, who);

	const uint32_t now = HAL_GetTick();
	if (0 == error_code)
	{
		// Этот товарищ ведет себя хорошо
		subor->ignorance_depth = 0;
		subor->last_good_report_time = now;
	}
	else
	{
		subor->last_mistake = error_code;

		subor->ignorance_depth++;
		subor->last_wrong_report_time = now;
	}
}


void commissar_work(void)
{
	commissar_t * const self = &commissar;

	for (size_t i = 0; i < COMMISSAR__SUBS_COUNT; i++)
	{
		subordinate_file_t * subor = _get_subor(self, i);
		// Если модуль явно замечен за тем, что он
		// сообщает об ошибке - будем сразу его исцелять
		if (subor->ignorance_depth > 0)
			_punish_the_heretic(self, i);
	}
}
