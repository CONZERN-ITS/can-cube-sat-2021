#ifndef INC_SENSORS_ITS_BME280_H_
#define INC_SENSORS_ITS_BME280_H_


#include <stdbool.h>
#include "main.h"

#include "mavlink_main.h"


typedef enum its_bme280_id_t
{
	ITS_BME_LOCATION_EXTERNAL = 0,
	ITS_BME_LOCATION_INTERNAL = 1,
} its_bme280_id_t;


struct its_bme280_t;
typedef struct its_bme280_t its_bme280_t;

//! Инициализация (или переинициализация) датчика
/*! Инициализация начинается с программной перезагрузки */
int its_bme280_reinit(its_bme280_id_t id);

//! Управление питанием датчика
void its_bme280_power(its_bme280_id_t id, bool enabled);

//! Получение mavlink пакета с данными BME
int its_bme280_read(its_bme280_id_t id, mavlink_pld_int_bme280_data_t * data);


#endif /* INC_SENSORS_ITS_BME280_H_ */
