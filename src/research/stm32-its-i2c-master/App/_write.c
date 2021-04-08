#include <stm32f1xx_hal.h>


extern UART_HandleTypeDef huart1;


int _write(int file, char *ptr, int len)
{
	HAL_UART_Transmit(&huart1, ptr, len, HAL_MAX_DELAY);
	return len;
}
