/*
 * driver_ms5611.c
 *
 *  Created on: Feb 5, 2021
 *      Author: developer
 */

#include "driver_ms5611.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "stm32f4xx_hal.h"


#define CMD_RESET	0x1E
#define CMD_READ_DATA	0x00

#define CMD_CONVERT_PRESSURE_OSR_256	0x40
#define CMD_CONVERT_PRESSURE_OSR_512	0x42
#define CMD_CONVERT_PRESSURE_OSR_1024	0x44
#define CMD_CONVERT_PRESSURE_OSR_2048	0x46
#define CMD_CONVERT_PRESSURE_OSR_4096	0x48
#define CMD_CONVERT_TEMP_OSR_256		0x50
#define CMD_CONVERT_TEMP_OSR_512		0x52
#define CMD_CONVERT_TEMP_OSR_1024		0x54
#define CMD_CONVERT_TEMP_OSR_2048		0x56
#define CMD_CONVERT_TEMP_OSR_4096		0x58

#define START_PROM_READ_ADDRESS		0b10100000


//last address bit is CSB pin value
#define MS5611_HIGH_I2C_ADDRESS		0b1110111
#define MS5611_LOW_I2C_ADDRESS		0b1110110


#define MS5611_I2C_INSTANCE	I2C1
#define MS5611_I2C_CLOCK_SPEED	400000

#define SCL GPIO_PIN_6
#define SDA GPIO_PIN_7

#define MS5611_I2C_FORCE_RESET 	__HAL_RCC_I2C1_FORCE_RESET
#define MS5611_I2C_RELEASE_RESET __HAL_RCC_I2C1_RELEASE_RESET


struct prom_data prom = {0};


I2C_HandleTypeDef ms5611_i2c = {
		.Instance = MS5611_I2C_INSTANCE,
		.Mode = HAL_I2C_MODE_MASTER,
		.Init = {
				.AddressingMode = I2C_ADDRESSINGMODE_7BIT,
				.ClockSpeed = MS5611_I2C_CLOCK_SPEED,
				.DualAddressMode = I2C_DUALADDRESS_DISABLE,
				.DutyCycle = I2C_DUTYCYCLE_2,
				.GeneralCallMode = I2C_GENERALCALL_DISABLE,
				.NoStretchMode = I2C_NOSTRETCH_DISABLE,
				.OwnAddress1 = 0x00
		}
};



int ms5611_reset(void * handle, uint16_t device_addr, uint8_t cmd)
{
	int error = 0;
	error = HAL_I2C_Master_Transmit(handle, device_addr, &cmd, sizeof(cmd), 10);
	return error;
}


int ms5611_read_prom_value(void * handle, uint16_t device_addr, uint8_t prom_start_addr, uint8_t prom_coef, uint16_t * prom_value)
{
	int error = 0;
	uint8_t prom_addr = prom_start_addr + prom_coef;
	error = HAL_I2C_Master_Transmit(handle, device_addr, &prom_addr , sizeof(prom_addr), 10);

	uint8_t bytes[2] = {0};
	error = HAL_I2C_Master_Receive(&ms5611_i2c, device_addr, bytes, sizeof(bytes), 100);

	*prom_value = ((uint16_t)bytes[0] << 1*8) | ((uint16_t)bytes[1] << 0*8);

	return error;
}


int ms5611_read_all_prom_data(void * handle, uint16_t device_addr, uint8_t prom_start_addr, struct prom_data * _prom)
{
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 1, &_prom->c1);
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 2, &_prom->c2);
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 3, &_prom->c3);
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 4, &_prom->c4);
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 5, &_prom->c5);
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 6, &_prom->c6);

	uint16_t prom_crc4 = 0;
	ms5611_read_prom_value(handle, device_addr, prom_start_addr, 7, &prom_crc4);

	//TODO:дописать проверку crc4

	return 0;
}


int ms5611_initiate_conversion(void * handle, uint16_t device_addr, uint8_t cmd)
{
	int error = 0;
	error = HAL_I2C_Master_Transmit(handle, device_addr, &cmd , sizeof(cmd), 10);
	return error;
}


int ms5611_read_data(void * handle, uint16_t device_addr, uint8_t cmd, uint32_t * data)
{
	int error = 0;
	error = HAL_I2C_Master_Transmit(handle, device_addr, &cmd , sizeof(cmd), 10);

	uint8_t bytes[3];
	error = HAL_I2C_Master_Receive(&ms5611_i2c, device_addr, bytes, sizeof(bytes), 10);
	if (error){
		// ...
	}
	*data = ((uint32_t)bytes[0] << 2*8) | ((uint32_t)bytes[1] << 1*8) | ((uint32_t)bytes[2] << 0*8);

	return error;

}


void ms5611_calculate_temp_and_pressure(uint32_t * raw_temp, uint32_t * raw_pressure, struct prom_data * _prom, int32_t * end_temp, int32_t * end_presssure)
{
	uint32_t r_temp = *raw_temp;
	uint32_t r_press = *raw_pressure;
	int32_t temp = 0;
	int32_t press = 0;

	int32_t dT = r_temp - _prom->c5 * pow(2, 8);		//Difference between actual and reference temperature
	temp = 2000 + (int32_t)(dT * _prom->c6 / pow(2, 23));		//Actual temperature (-40...85°C with 0.01°C resolution)

	int64_t off = _prom->c2 * pow(2, 16) + (_prom->c4 * dT) / pow(2, 7);		//Offset at actual temperature
	int64_t sens = _prom->c1 * pow(2, 15) + (_prom->c3 * dT) / pow(2, 8);		//Sensitivity at actual temperature



	if (temp < 2000)
	{
		int32_t t2 = dT*dT / pow(2, 31);
		int64_t off2, sens2;
		off2 = 5 * (temp - 2000)*(temp - 2000) / 2;
		sens2 = 5 * (temp - 2000)*(temp - 2000) / 2*2;

		if (temp < -1500)
		{
			off2 = off2 + 7 * (temp + 1500)*(temp + 1500);
			sens2 = sens2 + 11 * (temp + 1500)*(temp + 1500) / 2;
		}

		temp -= t2;
		off -= off2;
		sens -= sens2;
	}
	press = (r_press * sens / pow(2, 21) - off) / pow(2, 15);		//Temperature compensated pressure (10...1200mbar with 0.01mbar resolution)
	*end_presssure = press;
	*end_temp = temp;
}


//Code from AN520
unsigned char crc4(unsigned int n_prom[])
{
	int cnt;		// simple counter
	unsigned int n_rem;		// crc reminder
	unsigned int crc_read; 		// original value of the crc
	unsigned char  n_bit;
	n_rem = 0x00;
	crc_read = n_prom[7]; 		//save read CRC
	n_prom[7]=(0xFF00 & (n_prom[7]));		//CRC byte is replaced by 0
	for (cnt = 0; cnt < 16; cnt++)		// operation is performed on bytes
	{
		if (cnt%2==1)
			n_rem ^= (unsigned short) ((n_prom[cnt>>1]) & 0x00FF);
		else
			n_rem ^= (unsigned short) (n_prom[cnt>>1]>>8);

		for (n_bit = 8; n_bit > 0; n_bit--)
		{
			if (n_rem & (0x8000))
				n_rem = (n_rem << 1) ^ 0x3000;
			else
				n_rem = (n_rem << 1);
		}
	}

	n_rem = (0x000F & (n_rem >> 12));  // final 4-bit reminder is CRC code
	n_prom[7] = crc_read; 		// restore the crc_read to its original place
	return (n_rem ^ 0x0);		//return crc4 code
}
