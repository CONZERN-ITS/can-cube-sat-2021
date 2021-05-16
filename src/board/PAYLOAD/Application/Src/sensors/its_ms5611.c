#include <sensors/its_ms5611.h>
#include <assert.h>

#include <time_svc.h>
#include <util.h>

#include <sensors/ms5611.h>


extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;


#define ITS_MS5611_BUS_RCC_FORCE_RESET __HAL_RCC_I2C1_FORCE_RESET
#define ITS_MS5611_BUS_RCC_RELEASE_RESET __HAL_RCC_I2C1_RELEASE_RESET


#define ITS_MS5611_HAL_TIMEOUT (300)


struct its_ms5611_t
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
	ms5611_t driver;
	//! Калибровочка
	ms5611_prom_data_t prom;
	//! OSR для температуры
	ms5611_osr_t temp_osr;
	//! OSR для далвения
	ms5611_osr_t pres_osr;

	//! Код последней ошибки функций обмена
	HAL_StatusTypeDef last_error;
};


static its_ms5611_t * _dev_by_id(its_ms5611_id_t devid);


static int _write(void * user_arg, uint8_t command)
{
	// Поступим неможно грязно и в указателе будет хранить просто число - айдишник девайса
	its_ms5611_id_t devid = (its_ms5611_id_t)user_arg;

	its_ms5611_t * const device = _dev_by_id(devid);
	HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(
			device->bus, device->addr << 1, &command, 1, ITS_MS5611_HAL_TIMEOUT
	);

	if (HAL_OK != rc)
	{
		device->last_error = rc;
		return 1;
	}

	return 0;
}


static int _read(void * user_arg, void * data, size_t data_size)
{
	// Поступим неможно грязно и в указателе будет хранить просто число - айдишник девайса
	its_ms5611_id_t devid = (its_ms5611_id_t)user_arg;

	its_ms5611_t * const device = _dev_by_id(devid);
	HAL_StatusTypeDef rc = HAL_I2C_Master_Receive(
			device->bus, device->addr << 1, (uint8_t*)data, data_size, ITS_MS5611_HAL_TIMEOUT
	);

	if (HAL_OK != rc)
	{
		device->last_error = rc;
		return 1;
	}

	return 0;
}


static void _delay(void * user_arg, uint32_t ms)
{
	(void)user_arg;
	HAL_Delay(ms);
}


static void _power(its_ms5611_t * dev, bool enabled)
{
	HAL_GPIO_WritePin(dev->power_ctl_port, dev->power_ctl_pin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


static its_ms5611_t _devices[2] = {
		// Внешний
		{
			.bus = &hi2c1,
			.addr = MS5611_I2C_ADDR_HIGH,
			.power_ctl_port = MS_EXT_PWR_GPIO_Port, //GPIOC,
			.power_ctl_pin = MS_EXT_PWR_Pin, //GPIO_PIN_1,
			.driver = {
					._user_arg = (void*)ITS_MS_EXTERNAL,
					._write = _write,
					._read = _read,
					._delay = _delay
			},
			.temp_osr = MS5611_OSR_4096,
			.pres_osr = MS5611_OSR_4096,
			.last_error = HAL_OK
		},
		// Внутренний
		{
			.bus = &hi2c3,
			.addr = MS5611_I2C_ADDR_HIGH,
			.power_ctl_port = MS_INT_PWR_GPIO_Port, //GPIOB,
			.power_ctl_pin = MS_INT_PWR_Pin, //GPIO_PIN_0,
			.driver = {
				._user_arg = (void*)ITS_MS_INTERNAL,
				._write = _write,
				._read = _read,
				._delay = _delay
			},
			.temp_osr = MS5611_OSR_4096,
			.pres_osr = MS5611_OSR_4096,
			.last_error = HAL_OK
		}
};


static its_ms5611_t * _dev_by_id(its_ms5611_id_t devid)
{
	assert(devid >= 0 && devid < 2);
	its_ms5611_t * const retval = &_devices[(int)devid];
	return retval;
}


int its_ms5611_init(its_ms5611_id_t id)
{
	its_ms5611_t * const dev = _dev_by_id(id);
	_power(dev, true);

	// Сброс!
	int rc = ms5611_reset(&dev->driver);
	if (0 != rc)
		return rc;

	HAL_Delay(10);
	// Загрузка калиброви
	rc = ms5611_read_prom_data(&dev->driver, &dev->prom);
	if (0 != rc)
		return rc;

	return 0;
}


int its_ms5611_punish(its_ms5611_id_t id)
{
	its_ms5611_t * const dev = _dev_by_id(id);

	// рубим шину
	HAL_I2C_DeInit(dev->bus);
	// Рубим питание датчику
	_power(dev, false);

	HAL_Delay(10);

	_power(dev, true);
	HAL_I2C_Init(dev->bus);

	// Пробуем поднять датчик
	int rc;
	rc = its_ms5611_init(id);
	if (0 != rc)
		return rc;

	return 0;
}


int its_ms5611_read_and_calculate(its_ms5611_id_t id, mavlink_pld_ms5611_data_t * data)
{
	its_ms5611_t * const dev = _dev_by_id(id);

	struct timeval tv;
	time_svc_gettimeofday(&tv);

	int32_t temp;
	int32_t press;

	int rc = ms5611_read_compensated(&dev->driver, &dev->prom, dev->temp_osr, dev->pres_osr, &temp, &press);
	if (0 != rc)
		return rc;

	data->time_s = tv.tv_sec;
	data->time_us = tv.tv_usec;
	data->temperature = temp / 100;
	data->pressure = press;

	return 0;
}
