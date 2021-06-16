#include "clock-control-f4.h"

#include <stm32f4xx_hal.h>


int clock_control__high_clocks_setup_on_hsi(void)
{
	if (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSE)
	{
		if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_PLLCLK)
			return 0; // Мы и так работаем на PLL@HSE
	}


	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

	// На случай если HSI выключен - убежаемся в том что он включен
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 1;

	// На случай если мы сейчас почему-то едем на PLL - прыгаем с него на HSI
	if (RCC_SYSCLKSOURCE_STATUS_PLLCLK == __HAL_RCC_GET_SYSCLK_SOURCE())
		__HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

	// Запускаем PLL от HSI
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 2;

	// Прыгаем всем процом на PLL
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		return 3;

	return 0;
}


int clock_control__high_clocks_setup_on_hse(void)
{
	if (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSE)
	{
		if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_PLLCLK)
			return 0; // Мы и так работаем на PLL@HSE
	}

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

	// А давайте попробуем запустить HSE
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // PLL не трогаем
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 1; // Не вышлоу

	// Оно запустилось! На всякий случай спрыгиваем системной частотой с PLL
	if (RCC_SYSCLKSOURCE_STATUS_PLLCLK == __HAL_RCC_GET_SYSCLK_SOURCE())
		__HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

	// Перезапускем PLL на HSE
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 2; // Не вышлоу

	// Готовимся к прыжку на PLL@HSE
	// Если что-то вдруг пойдет не так, нужно чтобы кто-то нас спас
	HAL_RCC_EnableCSS();

	// Переставляем системную частоту на PLL
	// и расставляем все делители для периферии
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		return 3; // Не вышло и это честно говоря катастрофа

	return 0;
}


int clock_control__high_clocks_startup(void)
{
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	int rc = clock_control__high_clocks_setup_on_hsi();
	if (0 != rc)
	{
		// Это полная просто беда, так жить нельзя никак
		NVIC_SystemReset();
	}

	rc = clock_control__high_clocks_setup_on_hse();
	if (0 != rc)
	{
		// Неприятно, но жить можно, продолжаем
		return rc;
	}

	return 0;
}


static uint32_t _css_counter = 0;


int clock_control__high_clock_get_status(clock_control_status_t * status)
{
	switch(__HAL_RCC_GET_PLL_OSCSOURCE())
	{
	case RCC_PLLSOURCE_HSE:
		status->active_high_osc = CLOCK_CONTROL_HIGH_OSC_HSE;
		break;

	case RCC_PLLSOURCE_HSI:
		status->active_high_osc = CLOCK_CONTROL_HIGH_OSC_HSI;
		break;

	default:
		status->active_high_osc = CLOCK_CONTROL_HIGH_OSC_INVALID;
		break;

	}

	status->css_counter = _css_counter;

	return 0;
}


void HAL_RCC_CSSCallback()
{
	// Перекидывается на HSI
	clock_control__high_clocks_setup_on_hsi();
	_css_counter++;
}

