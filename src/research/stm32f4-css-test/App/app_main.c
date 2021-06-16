#include <stdio.h>

#include "main.h"
#include "clock-control-f4.h"


int app_main()
{
	int rc;
	printf("app main begun.\n");

	for(int i = 0; ; i++)
	{
		int osc_source = __HAL_RCC_GET_PLL_OSCSOURCE();
		int sys_source = __HAL_RCC_GET_SYSCLK_SOURCE();
		printf(
				"i'm here %d! pll_source: %d, sys_source: %d, css_counter\n",
				i, osc_source, sys_source
		);

		HAL_Delay(1000);
		if (i % 10 == 0)
		{
			rc = clock_control__high_clocks_setup_on_hse();
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
