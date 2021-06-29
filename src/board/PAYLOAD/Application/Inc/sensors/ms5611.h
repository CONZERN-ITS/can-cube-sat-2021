/*
 * driver_ms5611.h
 *
 *  Created on: Feb 5, 2021
 *      Author: developer
 */

#ifndef MS5611_DRIVER_MS5611_H_
#define MS5611_DRIVER_MS5611_H_


#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"


//! I2C адрес устройства в зависимости от ножки CSB
typedef enum ms5611_i2c_addr_t
{
	// CSB пин ВНИЗУ
	MS5611_I2C_ADDR_HIGH = 0x77,
	// CSB пин ВВЕРХУ
	MS5611_I2C_ADDR_LOW  = 0x76
} ms5611_i2c_addr_t;


//! Код ошибки драйвера
typedef enum ms5611_error_t
{
	//! Нет ошибки
	MS5611_ERROR_NONE = 0,
	//! Ошибка контрольной суммы
	MS5611_ERROR_CHECKSUM = 1,
	//! Ошибка функций обмена
	MS5611_ERROR_IO = 2
} ms5611_error_t;


//! Тип сенсора
typedef enum ms5611_sensor_t
{
	//! Термометр
	MS5611_SENSOR_TEMPERATURE  = 0x50,
	//! Манометр
	MS5611_SENSOR_PRESSURE     = 0x40,
} ms5611_sensor_t;


//! Оверсемплинг замеров
/*! Значения очевидны, ну */
typedef enum ms5611_osr_t
{
	MS5611_OSR_256   = 0x00,
	MS5611_OSR_512   = 0x02,
	MS5611_OSR_1024  = 0x04,
	MS5611_OSR_2048  = 0x06,
	MS5611_OSR_4096  = 0x08,
} ms5611_osr_t;


//! Параметры калибровки сенсора
/*! Значения расписаны в даташите */
typedef struct ms5611_prom_data_t {
	uint16_t c1;
	uint16_t c2;
	uint16_t c3;
	uint16_t c4;
	uint16_t c5;
	uint16_t c6;
} ms5611_prom_data_t;


//! Сигнатура функции для передачи команд
/*! \param user_arg пользовательский аргумент функции, хранимый в дескрипторе
    \param command байт команды, отправляемой на устройство */
typedef int(*ms5611_write_call_t)(void * _user_arg, uint8_t command);
//! Сигнатура функции для получения данных от сенсора
/*! \param user_arg пользовательский аргумент функции, хранимый в дескрипторе
    \param data указатель по которому следует писать запрашиваемые от устройства данные
    \param data_size количество запрашиваемых от устройства байт */
typedef int(*ms5611_read_call_t)(void * _user_arg, void * data, size_t data_size);
//! Сигнатура функции задержки
/*! \param user_arg пользовательский аргумент функции, хранимый в дескрипторе
    \param ms количество миллисекунд, сколько нужно ждать задержки */
typedef void (*ms5611_delay_call_t)(void * _user_arg, uint32_t ms);


//! дескриптор драйвера
typedef struct ms5611_t
{
	void * _user_arg;
	ms5611_write_call_t _write;
	ms5611_read_call_t  _read;
	ms5611_delay_call_t _delay;
} ms5611_t;


//! Инициализация хендла драйвера
/*! \param device хендл устройства
    \param write функция для передачи команд в устройство
    \param read функция для чтения данных из устройства
    \param rw_user_arg пользовательский аргумент, который будет передаваться функциям чтения и записи */
void ms5611_init_handle(ms5611_t * device,
		void * user_arg,
		ms5611_write_call_t write, ms5611_read_call_t read,
		ms5611_delay_call_t delay
);


//! программный сброс устройства
/*! \param device дескриптор устройства */
int ms5611_reset(ms5611_t * device);


//! Функция для чтения всех коэффициентов в памяти
/*  \param devuce дескриптор устройства
    \param data указатель на структуру, в которую будут считываться данные из prom памяти */
int ms5611_read_prom_data(ms5611_t * device, ms5611_prom_data_t * data);


//! Функция для отправки сигнала на начало формирования выходных данных
/*! \param device дескриптор устройства
    \param sensor указывает сенсор (термомент или манометр) замер по которому нужно обеспечить
    \param osr количество замеров по которым формируется значение */
int ms5611_initiate_conversion(ms5611_t * device, ms5611_sensor_t sensor, ms5611_osr_t osr);


//! Длительность замера в мс для указанного значения OSR
/*! Если на входе что-то не адекватное, на выходе будет
    \param osr значение OSR */
uint32_t ms5611_conversion_duration_for_osr(ms5611_osr_t osr);

//!	Функция для чтения данных.
/*! Данные формируются в течении некоторого времени (см datasheet),
    после отправки initiate_conversion
    \param device дескриптор устройства
    \param data указатеть на переменную, в которую необходимо положить результат */
int ms5611_read_data(ms5611_t * device, uint32_t * data);


//! Функция для пересчета сырых значений в реальные
/*! \param raw_temp сырые данные температуры, полученные из ms5611_read_data
    \param raw_pressure сырые данные давления, полученные из ms5611_read_data
    \param prom указатель на структуру с калибровочными данными, полученными из ms5611_read_all_prom_data
    \param end_temp указатель на переменную, в которую надо положить конечную температуру
    \param end_pressure указатель на переменную, в которую надо положить конечное давление */
void ms5611_calculate_temp_and_pressure(const ms5611_prom_data_t * prom,
		uint32_t raw_temp, uint32_t raw_pressure,
		int32_t * end_temp, int32_t * end_presssure
);


//! Функция все-в-одном
/*! Запускает измерение, ожидает его завершения, возвращает результат
    \param device дескриптор устройства
    \param prom набор калибровочных коэффициентов устройства
    \param temp_osr OSR для замера температуры
    \param pres_osr OSR для замера давления
    \oarar temp указатель на переменную для хранения результирующей темературы
    \param pres указатель на переменную для хранения результирующего давления
*/
int ms5611_read_compensated(ms5611_t * device, const ms5611_prom_data_t * prom,
		ms5611_osr_t temp_osr, ms5611_osr_t pres_osr,
		int32_t * temp, int32_t * pres
);

#endif /* MS5611_DRIVER_MS5611_H_ */
