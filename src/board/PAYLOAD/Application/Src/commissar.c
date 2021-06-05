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
#include "its-i2c-link.h"
#include "time_svc.h"


#define COMMISSAR_PUNISHMENT_COOLDOWN (5000)


//! Личное дело подчиненного комиссара
typedef struct subordinate_file_t
{
	//! Время последнего очета
	uint32_t last_report_time;
	//! Время последнего хорошего отчета
	uint32_t last_good_report_time;
	//! Сколько времени этому подчиненному можно не давать хороших отчетов
	uint32_t max_no_good_report_time;

	//! Общее количество отчетов подчиненного за всю карьеру комиссара
	uint32_t reports_counter;
	//! Общее количество плохих отчетов подчиенного за всю карьеру комиссара.
	uint32_t bad_reports_counter;

	//! Код последней ошибки
	int last_mistake;
	//! Длина серии последовательных ошибок
	uint32_t ignorance_depth;
	//! Сколько ошибок подряд можно совершить этому подчиненному, перед тем как
	//! комиссар начнет его перезапускать
	uint32_t max_ignorance_depth;

	//! Количество попыток поднятия модуля
	uint32_t punishments_counter;
	//! Метка времени последней попытки поднятия модуля
	uint32_t last_pusnishment_time;
	//! Пауза между наказаниями
	uint32_t punishment_cooldown;

} subordinate_file_t;


typedef struct commissar_t
{
	subordinate_file_t subors[COMMISSAR__SUBS_COUNT];
	size_t report_carret;
} commissar_t;


static commissar_t commissar;


static subordinate_file_t * _get_subor(commissar_t * const self, commissar_subordinate_t who)
{
	assert((int)who >= 0 && (int)who < COMMISSAR__SUBS_COUNT);
	return &self->subors[who];
}


static int _punish_the_heretic(commissar_t * const self, commissar_subordinate_t who)
{
	int rc = 0;
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

	case COMMISSAR_SUB_I2C_LINK:
		rc = its_i2c_link_reset();
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

	for (size_t i = 0; i < COMMISSAR__SUBS_COUNT; i++)
	{
		subordinate_file_t * const subor = _get_subor(self, i);
		memset(subor, 0x00, sizeof(*subor));
		// Почти всеми управляем по сигналу об ошибке
		// но не по времени
		subor->max_ignorance_depth = 0;
		subor->max_no_good_report_time = UINT32_MAX;
		subor->punishment_cooldown = COMMISSAR_PUNISHMENT_COOLDOWN;
	}

	// А вот I2C линк может ошибаться сколько захочет
	// Но не дольше указанного времени
	self->subors[COMMISSAR_SUB_I2C_LINK].max_no_good_report_time = 5*100;
	self->subors[COMMISSAR_SUB_I2C_LINK].max_ignorance_depth = UINT32_MAX;

	self->report_carret = 0;
}


void commissar_accept_report(commissar_subordinate_t who, int error_code)
{
	commissar_t * const self = &commissar;
	subordinate_file_t * const subor = _get_subor(self, who);

	const uint32_t now = HAL_GetTick();
	subor->last_report_time = now;
	subor->reports_counter++;
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
		subor->bad_reports_counter++;
	}
}


void commissar_provide_report(uint8_t * component_id, mavlink_commissar_report_t * report)
{
	commissar_t * const self = &commissar;
	const subordinate_file_t * const subor = _get_subor(self, self->report_carret);

	struct timeval tmv;
	time_svc_gettimeofday(&tmv);

	report->time_s = tmv.tv_sec;
	report->time_us = tmv.tv_usec;
	report->time_steady = HAL_GetTick();

	report->last_report_time = subor->last_report_time;
	report->last_good_report_time = subor->last_good_report_time;
	report->reports_counter = subor->reports_counter;
	report->bad_reports_counter = subor->bad_reports_counter;
	report->last_mistake = subor->last_mistake;
	report->ignorance_depth = subor->ignorance_depth;
	report->punishments_counter = subor->punishments_counter;
	report->last_punishment_time = subor->last_pusnishment_time;

	*component_id = (uint8_t)self->report_carret;
	self->report_carret++;
	if(self->report_carret >= COMMISSAR__SUBS_COUNT)
		self->report_carret = 0;
}


void commissar_work(void)
{
	commissar_t * const self = &commissar;

	const uint32_t now = HAL_GetTick();
	for (size_t i = 0; i < COMMISSAR__SUBS_COUNT; i++)
	{
		subordinate_file_t * subor = _get_subor(self, i);
		// Слишком часто не караем
		if (now - subor->last_pusnishment_time < subor->punishment_cooldown)
			continue;

		// Если модуль слишком погрузился в свое невежество и отрекся
		// от императора, будем его исцелять
		if (subor->ignorance_depth > subor->max_ignorance_depth)
			_punish_the_heretic(self, i);

		// Так же, если от модуля давно не было хороших вестей - так же будем
		// его исцелять
		else if (now - subor->last_good_report_time > subor->max_no_good_report_time)
			_punish_the_heretic(self, i);
	}
}
