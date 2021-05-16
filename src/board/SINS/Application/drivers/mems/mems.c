/*
 * mems.c
 *
 *  Created on: 8 апр. 2020 г.
 *      Author: snork
 */


#include "mems.h"

#include <assert.h>

#include "../common.h"

#include <stm32f4xx_hal.h>
#include <system_stm32f4xx.h>


void mems_power_off()
{
	HAL_GPIO_WritePin(PWR_MEMS_GPIO_Port, PWR_MEMS_Pin, RESET);
}


void mems_power_on()
{
	HAL_GPIO_WritePin(PWR_MEMS_GPIO_Port, PWR_MEMS_Pin, SET);
}


int mems_init_bus()
{
	//	SET_BIT(hmems_i2c.Instance->CR2, I2C_CR1_SWRST);
	//	CLEAR_BIT(hmems_i2c.Instance->CR1, I2C_CR1_SWRST);

	GPIO_InitTypeDef gpiob;
	gpiob.Mode = GPIO_MODE_INPUT;
	gpiob.Pin = SDA;		// SDA
	gpiob.Pull = GPIO_NOPULL;
	gpiob.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &gpiob);

	int pin = HAL_GPIO_ReadPin(GPIOB, SDA);

	if (pin == 0)		//если sda лежит, то будем клокать сами
	{
		HAL_I2C_MspDeInit(HMEMS_BUS_HANDLE);

		gpiob.Mode = GPIO_MODE_OUTPUT_OD;
		gpiob.Pin = SCL;		// SDA
		gpiob.Pull = GPIO_NOPULL;
		gpiob.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOB, &gpiob);

		scl_clocking(5);
	}

	HMEMS_I2C_FORCE_RESET();
	HMEMS_I2C_RELEASE_RESET();

	HAL_I2C_MspDeInit(HMEMS_BUS_HANDLE);
	mems_power_off();
	HAL_Delay(10);
	mems_power_on();
	HAL_Delay(10);
	HAL_I2C_MspInit(HMEMS_BUS_HANDLE);
	__HAL_I2C_RESET_HANDLE_STATE(HMEMS_BUS_HANDLE);

	HAL_StatusTypeDef hal_status =  HAL_I2C_Init(HMEMS_BUS_HANDLE);
	return sins_hal_status_to_errno(hal_status);
}


void mems_generate_stop_flag(void)
{
	SET_BIT(hmems_i2c.Instance->CR1,I2C_CR1_STOP);
}


static uint32_t get_dwt_count()
{
	return DWT->CYCCNT;
}


static void scl_up()
{
	HAL_GPIO_WritePin(GPIOB, SCL, SET);
}


static void scl_down()
{
	HAL_GPIO_WritePin(GPIOB, SCL, RESET);
}


void wait_need_ticks_delay(uint32_t ticks_delay)
{
	while (get_dwt_count() < ticks_delay)
	{
		volatile int x = 0;
		(void)x;
	}
}


void scl_clocking(int count_clocking)
{
	uint32_t need_count_tick_delay =  SystemCoreClock / (400000 * 2);
	for (int i = 0; i < count_clocking; i++)
	{
		DWT->CYCCNT = 0;
		scl_down();
		wait_need_ticks_delay(need_count_tick_delay);
		DWT->CYCCNT = 0;
		scl_up();
		wait_need_ticks_delay(need_count_tick_delay);
	}
}


int mems_get_gyro_staticShift(float* gyro_staticShift)
{
	int error = 0;
	uint16_t zero_orientCnt = 2000;

	//	Get static gyro shift
	float gyro[3] = {0, 0, 0};
	for (int i = 0; i < zero_orientCnt; i++)
	{
		//	Collect data
		int16_t raw_data[3] = {0};
		error = mems_lsm6ds3_get_g_data_raw(raw_data);
		mems_lsm6ds3_get_g_data_rps(raw_data, gyro);
		if (0 != error)
			return error;

		for (int m = 0; m < 3; m++)
			gyro_staticShift[m] += gyro[m];
	}


	for (int m = 0; m < 3; m++) {
		gyro_staticShift[m] /= zero_orientCnt;
	}


	return 0;
}


int mems_get_accel_staticShift(float* accel_staticShift)
{
	int error = 0;
	uint16_t zero_orientCnt = 300;


	float accel[3] = {0, 0, 0};
	for (int i = 0; i < zero_orientCnt; i++)
	{
		//	Collect data
		int16_t raw_data[3] = {0};
		error = mems_lsm6ds3_get_xl_data_raw(raw_data);
		mems_lsm6ds3_get_xl_data_g(raw_data, accel);
		if (0 != error)
			return error;

		//	Set accel static shift vector as (0,0,g)
		accel_staticShift[0] = 0;
		accel_staticShift[1] = 0;
		accel_staticShift[2] += sqrt(accel[0]*accel[0] + accel[1]*accel[1] + accel[2]*accel[2]);
	}

	//	Divide shift by counter
	for (int m = 0; m < 3; m++)
		accel_staticShift[m] /= zero_orientCnt;


	return 0;
}
