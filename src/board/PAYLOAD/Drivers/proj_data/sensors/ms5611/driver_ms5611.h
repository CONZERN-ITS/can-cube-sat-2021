/*
 * driver_ms5611.h
 *
 *  Created on: Feb 5, 2021
 *      Author: developer
 */

#ifndef MS5611_DRIVER_MS5611_H_
#define MS5611_DRIVER_MS5611_H_


#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef ms5611_i2c;


struct prom_data {
	uint16_t c1;
	uint16_t c2;
	uint16_t c3;
	uint16_t c4;
	uint16_t c5;
	uint16_t c6;
};


/*	Общие переменные:
 * 	1) handle - указатеть на структуру I2C_HandleTypeDef для шины датчика
 * 	2) device_addr - адрес утройства на шине
 * 	все int функции возвращают ошибки
 */

/*	Функция для резета
 * 	3) cmd - команда на переинициализацию датчика (CMD_RESET)
 */
extern int ms5611_reset(void * handle, uint16_t device_addr, uint8_t cmd);


/*	Функция для чтения всех коэффициентов в памяти
 * 	3) prom_start_addr - адрес начала prom памяти в датчике (START_PROM_READ_ADDRESS)
 * 	4) _prom - указатель на структуру, в которую будут считываться данные из prom памяти
 */
extern int ms5611_read_all_prom_data(void * handle, uint16_t device_addr, uint8_t prom_start_addr, struct prom_data * _prom);


/*	Функция для отправки сигнала на начало формирования выходных данных
 * 	Необходимо выполнять функцию и для температуры и для давления
 *	3) cmd - команда для начала формирования дянных (like CMD_CONVERT_PRESSURE_OSR_256 or CMD_CONVERT_TEMP_OSR_256 or another)
 */
extern int ms5611_initiate_conversion(void * handle, uint16_t device_addr, uint8_t cmd);


/*	Функция для чтения данных. Данные формируются в течении некоторого времени (см datasheet),
 * после отправки initiate_conversion
 * 	3) cmd - команда для чтения данных (CMD_READ_DATA)
 * 	4) data - указатеть на переменную, в которую необходимо положить результат
 */
extern int ms5611_read_data(void * handle, uint16_t device_addr, uint8_t cmd, uint32_t * data);


/*	Функция для пересчета сырых значений в реальные
 * 	1) raw_temp - сырые данные температуры, полученные из ms5611_read_data
 * 	2) raw_pressure - сырые данные давления, полученные из ms5611_read_data
 * 	3) _prom - указатель на структуру с калибровочными данными, полученными из ms5611_read_all_prom_data
 * 	4) end_temp - указатель на переменную, в которую надо положить конечную температуру
 * 	5) end_pressure - указатель на переменную, в которую надо положить конечное давление
 */
extern void ms5611_calculate_temp_and_pressure(uint32_t * raw_temp, uint32_t * raw_pressure, struct prom_data * _prom, int32_t * end_temp, int32_t * end_presssure);


#endif /* MS5611_DRIVER_MS5611_H_ */
