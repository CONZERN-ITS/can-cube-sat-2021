/*
 * adc.c
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#include <analog/analog.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include <stm32f4xx_hal.h>

#include "drivers/common.h"


extern ADC_HandleTypeDef hadc1;

#define _ADC_HAL_TIMEOUT (HAL_MAX_DELAY)

#define _ADC_HANDLE (&hadc1)


int analog_init(void)
{
	// Включаем АЦП
	__HAL_ADC_ENABLE(&hadc1);
	return 0;
}


static int _channgel_config_for_target(analog_target_t target, ADC_ChannelConfTypeDef * config)
{
	config->Offset = 0;
	config->Rank = 1;

	switch (target)
	{
	case ANALOG_TARGET_INTEGRATED_TEMP:
		config->Channel = ADC_CHANNEL_TEMPSENSOR;
		break;

	case ANALOG_TARGET_VBAT:
		config->Channel = ADC_CHANNEL_VBAT;
		break;

	case ANALOG_TARGET_LED_0:
		config->Channel = ADC_CHANNEL_1;
		break;

	case ANALOG_TARGET_LED_1:
		config->Channel = ADC_CHANNEL_4;
		break;

	case ANALOG_TARGET_LED_2:
		config->Channel = ADC_CHANNEL_5;
		break;

	case ANALOG_TARGET_LED_3:
		config->Channel = ADC_CHANNEL_8;
		break;

	case ANALOG_TARGET_LED_4:
		config->Channel = ADC_CHANNEL_9;
		break;

	case ANALOG_TARGET_LED_5:
		config->Channel = ADC_CHANNEL_10;
		break;

	case ANALOG_TARGET_LED_6:
		config->Channel = ADC_CHANNEL_11;
		break;

	case ANALOG_TARGET_LED_7:
		config->Channel = ADC_CHANNEL_12;
		break;

	case ANALOG_TARGET_LED_8:
		config->Channel = ADC_CHANNEL_14;
		break;

	case ANALOG_TARGET_LED_9:
		config->Channel = ADC_CHANNEL_15;
		break;

	default:
		return -ENODEV;
	}

	if (target == ANALOG_TARGET_INTEGRATED_TEMP)
		config->SamplingTime = ADC_SAMPLETIME_480CYCLES;
	else
		config->SamplingTime = ADC_SAMPLETIME_144CYCLES;

	int error = HAL_ADC_ConfigChannel(&hadc1, config);
	return error;
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
	int rc = sins_hal_status_to_errno(hal_rc);
	if (0 != rc)
		return rc;

	return analog_init();
}



int analog_get_raw(analog_target_t target, uint16_t oversampling, uint16_t * value)
{
	ADC_ChannelConfTypeDef vtref_config;
	vtref_config.Channel = ADC_CHANNEL_VREFINT;
	vtref_config.Offset = 0;
	vtref_config.Rank = 1;
	vtref_config.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	ADC_ChannelConfTypeDef target_config;
	HAL_StatusTypeDef error = _channgel_config_for_target(target, &target_config);
	if (0 != error)
		return error;

	uint32_t raw_sum = 0;
	uint32_t vrefint_sum = 0;
	for (uint16_t i = 0; i < oversampling; i++)
	{
		// Замер целевого напряжения
		HAL_StatusTypeDef error = HAL_ADC_ConfigChannel(_ADC_HANDLE, &target_config);
		if (HAL_OK != error)
			return error;

		error = HAL_ADC_Start(&hadc1);
		if (HAL_OK != error)
			return error;

		error = HAL_ADC_PollForConversion(_ADC_HANDLE, _ADC_HAL_TIMEOUT);
		if (HAL_OK != error)
			return error;

		raw_sum += HAL_ADC_GetValue(&hadc1);


		// Замер вдда
		error = HAL_ADC_ConfigChannel(_ADC_HANDLE, &vtref_config);
		if (HAL_OK != error)
			return error;

		error = HAL_ADC_Start(&hadc1);
		if (HAL_OK != error)
			return error;

		error = HAL_ADC_PollForConversion(_ADC_HANDLE, _ADC_HAL_TIMEOUT);
		if (HAL_OK != error)
			return error;

		vrefint_sum += HAL_ADC_GetValue(&hadc1);
	}

	uint32_t retval = raw_sum / oversampling;
	uint32_t vrefint = vrefint_sum / oversampling;

	// Корректируем с учетом внешнего напряжения
	retval = retval * (*VREFINT_CAL_ADDR) / vrefint ;

	*value = (uint16_t)retval;
	return 0;
}


int analog_get_vdda_mv(uint16_t oversampling, uint16_t * value)
{
	ADC_ChannelConfTypeDef vtref_config;
	vtref_config.Channel = ADC_CHANNEL_VREFINT;
	vtref_config.Offset = 0;
	vtref_config.Rank = 1;
	vtref_config.SamplingTime = ADC_SAMPLETIME_480CYCLES;

	HAL_StatusTypeDef error = HAL_ADC_ConfigChannel(_ADC_HANDLE, &vtref_config);
	if (HAL_OK != error)
		return error;

	uint32_t raw_sum = 0;
	for (uint16_t i = 0; i < oversampling; i++)
	{
		error = HAL_ADC_Start(&hadc1);
		if (HAL_OK != error)
			return error;

		error = HAL_ADC_PollForConversion(_ADC_HANDLE, _ADC_HAL_TIMEOUT);
		if (HAL_OK != error)
			return error;

		raw_sum += HAL_ADC_GetValue(&hadc1);
	}

	uint32_t vrefint = raw_sum / oversampling;
	uint32_t retval = VREFINT_CAL_VREF * (*VREFINT_CAL_ADDR) / (vrefint);
	*value = (uint16_t)retval;
	return 0;
}


