/*
 * adc.c
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#include "sensors/analog.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include <stm32f4xx_hal.h>

#include "util.h"


extern ADC_HandleTypeDef hadc1;

#define _ADC_HAL_TIMEOUT (HAL_MAX_DELAY)
#define _ADC_HANDLE (&hadc1)


//! Создает структуру конфигурации канала АЦП
static int _channgel_config_for_target(analog_target_t target, ADC_ChannelConfTypeDef * config)
{
	// Все согласно разводке на плате
	int error = 0;
	switch(target)
	{
	case ANALOG_TARGET_MICS6814_NO2:
		config->Channel = ADC_CHANNEL_0;
		config->Rank = 1;
		config->SamplingTime = ADC_SAMPLETIME_480CYCLES;
		break;

	case ANALOG_TARGET_MICS6814_NH3:
		config->Channel = ADC_CHANNEL_1;
		config->Rank = 1;
		config->SamplingTime = ADC_SAMPLETIME_480CYCLES;
		break;

	case ANALOG_TARGET_MICS6814_CO:
		config->Channel = ADC_CHANNEL_2;
		config->Rank = 1;
		config->SamplingTime = ADC_SAMPLETIME_480CYCLES;
		break;

	case ANALOG_TARGET_ME202_O2:
		config->Channel = ADC_CHANNEL_3;
		config->Rank = 1;
		config->SamplingTime = ADC_SAMPLETIME_480CYCLES;
		break;

	case ANALOG_TARGET_DNA_TEMP:
		config->Channel = ADC_CHANNEL_4;
		config->Rank = 1;
		config->SamplingTime = ADC_SAMPLETIME_144CYCLES;
		break;

	case ANALOG_TARGET_INTEGRATED_TEMP:
		config->Channel = ADC_CHANNEL_TEMPSENSOR;
		config->Rank = 1;
		// Если АЦП работает на 84Мгц APB2 шины с делителем 6
		// то один его цикл занимает 1/(82*10**6/6)
		// Чтобы адекватно замерять vtref, нужно мерять его минимум 10 мкс (10*1e-6)
		// И для этого нужно минимум (10*1e-6)/(1/(82*10**6/6)) == 140 циклов АЦП
		config->SamplingTime = ADC_SAMPLETIME_144CYCLES;
		break;

	default:
		error = -ENOSYS;
		break;
	}

	return error;
}


int analog_init()
{
	// Включаем АЦП
	__HAL_ADC_ENABLE(&hadc1);

	return 0;
}


int analog_restart(void)
{
	// Глушим ADC к чертям
	HAL_ADC_DeInit(&hadc1);
	__HAL_RCC_ADC_FORCE_RESET();
	__HAL_RCC_ADC_RELEASE_RESET();
	__HAL_ADC_RESET_HANDLE_STATE(&hadc1);

	// Включаем

	HAL_StatusTypeDef hal_rc;
	hal_rc = HAL_ADC_Init(&hadc1);
	int rc = hal_status_to_errno(hal_rc);
	if (0 != rc)
		return rc;

	return analog_init();
}


int analog_get_raw(analog_target_t target, uint16_t * value)
{
	int error = 0;

	ADC_ChannelConfTypeDef config;
	error = _channgel_config_for_target(target, &config);
	if (0 != error)
		return error;

	HAL_StatusTypeDef hal_error = HAL_ADC_ConfigChannel(_ADC_HANDLE, &config);
	error = hal_status_to_errno(hal_error);
	if (0 != error)
		return error;

	hal_error = HAL_ADC_Start(&hadc1);
	error = hal_status_to_errno(hal_error);
	if (0 != error)
		return error;

	hal_error = HAL_ADC_PollForConversion(_ADC_HANDLE, _ADC_HAL_TIMEOUT);
	error = hal_status_to_errno(hal_error);
	if (0 != error)
		return error;

	*value = (uint16_t)HAL_ADC_GetValue(_ADC_HANDLE);
	return 0;
}


int analog_get_raw_with_vdda(analog_target_t target, uint16_t * value, uint16_t * vdda)
{
	int rc;
	rc = analog_get_raw(target, value);
	if (0 != rc)
		return rc;

	rc = analog_get_vdda_mv(vdda);
	if (0 != rc)
		return rc;

	return 0;
}


int analog_get_vdda_mv(uint16_t * value)
{
	ADC_ChannelConfTypeDef cfg;
	cfg.Channel = ADC_CHANNEL_VREFINT;
	cfg.Rank = 1;
	// Если АЦП работает на 84Мгц APB2 шины с делителем 6
	// то один его цикл занимает 1/(82*10**6/6)
	// Чтобы адекватно замерять vtref, нужно мерять его минимум 10 мкс (10*1e-6)
	// И для этого нужно минимум (10*1e-6)/(1/(82*10**6/6)) == 140 циклов АЦП
	cfg.SamplingTime = ADC_SAMPLETIME_144CYCLES;

	HAL_ADC_ConfigChannel(_ADC_HANDLE, &cfg);
	HAL_ADC_Start(_ADC_HANDLE);
	HAL_ADC_PollForConversion(_ADC_HANDLE, _ADC_HAL_TIMEOUT);

	uint16_t vtref_value = (uint16_t)HAL_ADC_GetValue(_ADC_HANDLE);
	uint32_t vdda_mv = (uint32_t)VREFINT_CAL_VREF * (*VREFINT_CAL_ADDR) / vtref_value;
	*value = (uint16_t)vdda_mv;

	return 0;
}


