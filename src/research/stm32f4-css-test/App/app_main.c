#include <stdio.h>

#include "main.h"

int switch_to_hse(void)
{
	if (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSE)
	{
		if (__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_PLLCLK)
			return 0; // Мы и так работаем на HSE
	}

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

	// Пробуем его запустить
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // PLL не трогаем
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 1; // Не вышлоу

	// Оно запустилось! Спрыгиваем системной частотой с PLL
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


int init_clock_hsi(void)
{
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	// Включаем HSI и сразу же запитываем от него PLL
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		return 1;

	// Прыгаем все процом на PLL
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		return 2;

	return 0;
}


static uint32_t _css_counter = 0;

void HAL_RCC_CSSCallback()
{
	// Перекидывается на HSI
	int rc = init_clock_hsi();
	_css_counter++;
}


int app_main()
{
	int rc;
	printf("app main beguns. Going to HSE\n");

	rc = switch_to_hse();
	printf("hse init: %d\n", rc);

	for(int i = 0; ; i++)
	{
		int osc_source = __HAL_RCC_GET_PLL_OSCSOURCE();
		int sys_source = __HAL_RCC_GET_SYSCLK_SOURCE();
		printf(
				"i'm here %d! pll_source: %d, sys_source: %d, css_counter: %d\n",
				i, osc_source, sys_source, (int)_css_counter
		);

		HAL_Delay(1000);
		if (i % 10 == 0)
		{
			rc = switch_to_hse();
			printf("switching to hse: %d\n", rc);
		}
	}

	return 0;
}


int _write(int file, char *ptr, int len)
{
	extern UART_HandleTypeDef huart1;

	HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
	return len;
}
