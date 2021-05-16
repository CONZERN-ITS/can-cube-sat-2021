/*
 * syscalls.c
 *
 *  Created on: 14 июл. 2020 г.
 *      Author: developer
 */

#include <assert.h>
#include <stm32f4xx_hal.h>


extern UART_HandleTypeDef huart3;
int
_write(int file, char* ptr, int len)
{
	assert(0 == file);

	HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
	return len;
}

int
_close(int fildes __attribute__((unused)))
{
	assert(0);
	return -1;
}

int
_read(int file __attribute__((unused)), char* ptr __attribute__((unused)),
    int len __attribute__((unused)))
{
	assert(0);
	return -1;
}

int _lseek(int file __attribute__((unused)), int ptr __attribute__((unused)), int dir __attribute__((unused)))
{
	assert(0);
	return 0;
}
