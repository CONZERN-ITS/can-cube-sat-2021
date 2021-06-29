#include "sensors/its_bme280.h"

#include <stdint.h>
#include <assert.h>

#include <stm32f4xx_hal.h>

#include "icao_table_calc.h"

#include "bme280.h"
#include "util.h"
#include "time_svc.h"


extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c1;


#define ITS_BME_HAL_TIMEOUT (300)


struct its_bme280_t
{
	//! Шина I2C на которой датчик работает
	I2C_HandleTypeDef * bus;
	//! Адрес датчика на шине
	uint8_t addr;
	//! Порт, на котором стоит транзистор, управляющий питанием датчика
	GPIO_TypeDef * power_ctl_port;
	//! Пин порта, , на котором стоит транзистор, управляющий питанием датчика
	uint32_t power_ctl_pin;
	//! Структура родного драйвера
	struct bme280_dev driver;
};


static its_bme280_t * _dev_by_id(its_bme280_id_t id);
static int8_t _i2c_read(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
static int8_t _i2c_write(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len);
static void _delay_ms(uint32_t ms);


static its_bme280_t _devices[2] = {
	{
		// Внешний BME280
		.addr = BME280_I2C_ADDR_PRIM,
		.bus = &hi2c3,
		.power_ctl_port = BME_EXT_PWR_GPIO_Port, //GPIOB,
		.power_ctl_pin = BME_EXT_PWR_Pin, //GPIO_PIN_1,
		.driver = {
			.dev_id = ITS_BME_LOCATION_EXTERNAL,
			.intf = BME280_I2C_INTF,
			.read = _i2c_read,
			.write = _i2c_write,
			.delay_ms = _delay_ms,
			.settings = {
					.filter = BME280_FILTER_COEFF_OFF,
					.osr_h = BME280_OVERSAMPLING_16X,
					.osr_p = BME280_OVERSAMPLING_16X,
					.osr_t = BME280_OVERSAMPLING_16X,
					.standby_time = BME280_STANDBY_TIME_500_MS
			}
		}
	},
	{
		// Внутренний BME280
		.addr = BME280_I2C_ADDR_PRIM,
		.bus = &hi2c1,
		.power_ctl_port = BME_INT_PWR_GPIO_Port, //GPIOC,
		.power_ctl_pin = BME_EXT_PWR_Pin, //GPIO_PIN_0,
		.driver = {
			.dev_id = ITS_BME_LOCATION_INTERNAL,
			.intf = BME280_I2C_INTF,
			.read = _i2c_read,
			.write = _i2c_write,
			.delay_ms = _delay_ms,
			.settings = {
					.filter = BME280_FILTER_COEFF_OFF,
					.osr_h = BME280_OVERSAMPLING_16X,
					.osr_p = BME280_OVERSAMPLING_16X,
					.osr_t = BME280_OVERSAMPLING_16X,
					.standby_time = BME280_STANDBY_TIME_500_MS
			}
		}
	}
};


static its_bme280_t * _dev_by_id(its_bme280_id_t id)
{
	assert(id >= 0 && id < 2);
	its_bme280_t * const dev = &_devices[id];

	return dev;
}


static int8_t _i2c_read(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	its_bme280_t * dev = _dev_by_id(id);

	HAL_StatusTypeDef hrc = HAL_I2C_Mem_Read(
			dev->bus,
			dev->addr << 1,
			reg_addr,
			1,
			data,
			len,
			ITS_BME_HAL_TIMEOUT
	);

	int rc = hal_status_to_errno(hrc);
	return rc;
}


static int8_t _i2c_write(uint8_t id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	its_bme280_t * dev = _dev_by_id(id);

	HAL_StatusTypeDef hrc = HAL_I2C_Mem_Write(
			dev->bus,
			dev->addr << 1,
			reg_addr,
			1,
			data,
			len,
			ITS_BME_HAL_TIMEOUT
	);

	int rc = hal_status_to_errno(hrc);
	return rc;
}



static void _delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}


static void _power(its_bme280_t * dev, bool enabled)
{
	HAL_GPIO_WritePin(dev->power_ctl_port, dev->power_ctl_pin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}



int its_bme280_init(its_bme280_id_t id)
{
	its_bme280_t * const dev = _dev_by_id(id);
	_power(dev, true);

	int rc = bme280_soft_reset(&dev->driver);
	if (0 != rc)
		return rc;

	rc = bme280_init(&dev->driver);
	if (0 != rc)
		return rc;

	rc = bme280_set_sensor_settings(BME280_ALL_SETTINGS_SEL, &dev->driver);
	if (0 != rc)
		return rc;

	rc = bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev->driver);
	if (0 != rc)
		return rc;

	HAL_Delay(10); // Иначе первые данные получаются плохие
	return 0;
}


int its_bme280_punish(its_bme280_id_t id)
{
	its_bme280_t * const dev = _dev_by_id(id);

	// Будем действовать радикально и будем сразу стрелять шину
	HAL_I2C_DeInit(dev->bus);
	_power(dev, false);
	HAL_Delay(10);

	_power(dev, true);
	HAL_I2C_Init(dev->bus);

	// Пробуем перезапустить датчик
	int rc;
	rc = its_bme280_init(id);
	if (0 != rc)
		return rc;

	return 0;
}


int its_bme280_read(its_bme280_id_t id, mavlink_pld_bme280_data_t * data)
{
	its_bme280_t * const dev = _dev_by_id(id);

	struct bme280_data bme280_data;

	struct timeval tv;
	time_svc_gettimeofday(&tv);

	int rc = bme280_get_sensor_data(BME280_ALL, &bme280_data, &dev->driver);
	if (0 != rc)
		return rc;

	data->time_s = tv.tv_sec;
	data->time_us = tv.tv_usec;
	data->time_steady = HAL_GetTick();

	data->pressure = bme280_data.pressure;
	data->temperature = bme280_data.temperature;
	data->humidity = bme280_data.humidity;
	data->altitude = icao_table_alt_for_pressure((float)bme280_data.pressure);
	return 0;
}
