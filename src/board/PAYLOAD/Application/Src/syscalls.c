
#include <stm32f4xx_hal.h>

extern UART_HandleTypeDef huart4;

int _write(int file, char *ptr, int len)
{

	HAL_UART_Transmit(&huart4, (uint8_t * )ptr, len, HAL_MAX_DELAY);
	return len;
}
