/*
 * driver_ms5611.c
 *
 *  Created on: Feb 5, 2021
 *      Author: developer
 */

#include <sensors/ms5611.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <stm32f4xx_hal.h>


typedef enum ms5611_cmd_t
{
	MS5611_CMD_RESET = 0x1E,
	MS5611_CMD_ADC_READ = 0x00,

	MS5611_CMD_CONVERT_D1_OSR_256  = 0x40,
	MS5611_CMD_CONVERT_D1_OSR_512  = 0x42,
	MS5611_CMD_CONVERT_D1_OSR_1024 = 0x44,
	MS5611_CMD_CONVERT_D1_OSR_2048 = 0x46,
	MS5611_CMD_CONVERT_D1_OSR_4096 = 0x48,

	MS5611_CMD_CONVERT_D2_OSR_256  = 0x50,
	MS5611_CMD_CONVERT_D2_OSR_512  = 0x52,
	MS5611_CMD_CONVERT_D2_OSR_1024 = 0x54,
	MS5611_CMD_CONVERT_D2_OSR_2048 = 0x56,
	MS5611_CMD_CONVERT_D2_OSR_4096 = 0x58,

	// Это команда особая и на самом деле задает диапазон команд
	// от 0xA0 до 0xAE для чтения отдельных регистров.
	// Поэтому младшие биты тут работают как адрес PROM регистра
	// При этом самый младший бит всегда 0, то есть
	// адрес регистра в эту команду передается через << 1
	MS5611_CMD_PROM_READ_OFFSET = 0xA0
} ms5611_cmd_t;



//Code from AN520, modified
static uint8_t _crc4(uint16_t n_prom[])
{
	int cnt;		// simple counter
	uint16_t n_rem;		// crc reminder
	uint16_t crc_read; 		// original value of the crc
	uint8_t n_bit;
	n_rem = 0x00;
	crc_read = n_prom[7]; 		//save read CRC
	n_prom[7]=(0xFF00 & (n_prom[7]));		//CRC byte is replaced by 0
	for (cnt = 0; cnt < 16; cnt++)		// operation is performed on bytes
	{
		if (cnt%2==1)
			n_rem ^= (uint16_t) ((n_prom[cnt>>1]) & 0x00FF);
		else
			n_rem ^= (uint16_t) (n_prom[cnt>>1]>>8);

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


void ms5611_init_handle(ms5611_t * device,
		void * user_arg,
		ms5611_write_call_t write, ms5611_read_call_t read,
		ms5611_delay_call_t delay
) {
	memset(device, 0x00, sizeof(*device));
	device->_user_arg = user_arg;
	device->_read = read;
	device->_write = write;
	device->_delay = delay;
}


int ms5611_reset(ms5611_t * device)
{
	int io_error = device->_write(device->_user_arg, MS5611_CMD_RESET);
	if (io_error != 0)
		return MS5611_ERROR_IO;

	return MS5611_ERROR_NONE;
}


int ms5611_read_prom_data(ms5611_t * device, ms5611_prom_data_t * data)
{
	uint16_t prom_data[8]; // Шесть регистров prom + резервированный байт в начале + crc в конце
	for (int i = 0; i < sizeof(prom_data); i++) // 6 регистров в проме + crc
	{
		// Адрес регистра тут нужно сдвигать на 1
		// т.к. для адреса регистра используются биты 4 5 6 (считая со старшего) и не используется бит 7
		// Короче говоря адрес нужно сдвигать на правильную позицию
		int io_error = device->_write(device->_user_arg, MS5611_CMD_PROM_READ_OFFSET | (i << 1));
		if (0 != io_error)
			return MS5611_ERROR_IO;

		uint8_t bytes[2];
		io_error = device->_read(device->_user_arg, bytes, sizeof(bytes));
		if (0 != io_error)
			return MS5611_ERROR_IO;

		prom_data[i] = (bytes[0] << 8) | bytes[1];
	}

	// Проверяем контрольную сумму
	uint8_t crc = _crc4(prom_data);
	if (0 != crc)
		return MS5611_ERROR_CHECKSUM;

	// Копируем данные пользователю
	// Начинаем с 1, так как первый регистр - резевр под чего-то там
	data->c1 = prom_data[1];
	data->c2 = prom_data[2];
	data->c3 = prom_data[3];
	data->c4 = prom_data[4];
	data->c5 = prom_data[5];
	data->c6 = prom_data[6];

	return MS5611_ERROR_NONE;
}


int ms5611_initiate_conversion(ms5611_t * device, ms5611_sensor_t sensor, ms5611_osr_t osr)
{
	uint8_t byte = ((uint8_t)sensor << 4) | ((uint8_t)osr << 0);
	int io_error = device->_write(device->_user_arg, byte);
	if (io_error != 0)
		return MS5611_ERROR_IO;

	return MS5611_ERROR_NONE;
}


uint32_t ms5611_conversion_duration_for_osr(ms5611_osr_t osr)
{
	// Цифры взяты из даташита несколько твораческим образом

	switch (osr)
	{
	case MS5611_OSR_256:
		return 1;
	case MS5611_OSR_512:
		return 2;
	case MS5611_OSR_1024:
		return 3;
	case MS5611_OSR_2048:
		return 5;
	case MS5611_OSR_4096:
		return 10;
	};

	// Какая-то фигня
	return 0;
}


int ms5611_read_data(ms5611_t * device, uint32_t * data)
{
	uint8_t bytes[3];
	int io_error = device->_read(device, bytes, sizeof(bytes));
	if (io_error != 0)
		return MS5611_ERROR_IO;

	*data = ((uint32_t)bytes[0] << 2*8) | ((uint32_t)bytes[1] << 1*8) | ((uint32_t)bytes[2] << 0*8);
	return MS5611_ERROR_NONE;
}


void ms5611_calculate_temp_and_pressure(const ms5611_prom_data_t * prom,
		uint32_t raw_temp, uint32_t raw_pressure,
		int32_t * end_temp, int32_t * end_presssure
)
{
	uint32_t r_temp = raw_temp;
	uint32_t r_press = raw_pressure;
	int32_t temp = 0;
	int32_t press = 0;

	int32_t dT = r_temp - prom->c5 * pow(2, 8);		//Difference between actual and reference temperature
	temp = 2000 + (int32_t)(dT * prom->c6 / pow(2, 23));		//Actual temperature (-40...85°C with 0.01°C resolution)

	int64_t off = prom->c2 * pow(2, 16) + (prom->c4 * dT) / pow(2, 7);		//Offset at actual temperature
	int64_t sens = prom->c1 * pow(2, 15) + (prom->c3 * dT) / pow(2, 8);		//Sensitivity at actual temperature

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


int ms5611_read_compensated(ms5611_t * device, const ms5611_prom_data_t * prom,
		ms5611_osr_t temp_osr, ms5611_osr_t pres_osr,
		int32_t * temp, int32_t * pres
)
{
	// Меряем температуру
	int io_error = ms5611_initiate_conversion(device, MS5611_SENSOR_TEMPERATURE, temp_osr);
	if (io_error != 0)
		return MS5611_ERROR_IO;

	// Ждем заверешния
	device->_delay(device->_user_arg, ms5611_conversion_duration_for_osr(temp_osr));

	// Вычитываем
	uint32_t raw_temp;
	io_error = ms5611_read_data(device, &raw_temp);
	if (io_error != 0)
		return io_error;

	// Меряем давление
	io_error = ms5611_initiate_conversion(device, MS5611_SENSOR_PRESSURE, pres_osr);
	if (io_error != 0)
		return MS5611_ERROR_IO;

	// Снова ждем завершения
	device->_delay(device->_user_arg, ms5611_conversion_duration_for_osr(pres_osr));

	// Снова вычитываем
	uint32_t raw_pres;
	io_error = ms5611_read_data(device, &raw_pres);
	if (io_error != 0)
		return io_error;


	// Считаем
	ms5611_calculate_temp_and_pressure(prom, raw_temp, raw_pres, temp, pres);


	// Мы великолепны
	return MS5611_ERROR_NONE;
}
