
#ifndef INC_SENSORS_INT_BME_H_
#define INC_SENSORS_INT_BME_H_

#include "mavlink_main.h"


//! Инициализация сенсора
int int_bme_init(void);

//! Реинициализация сенсора.
//! Сброс всего до чего можно дотянуться и повторная попытка инициализации
int int_bme_restart(void);

//! Получение mavlink пакета с данными BME
int int_bme_read(mavlink_pld_int_bme280_data_t * data);


#endif /* INC_SENSORS_INT_BME_H_ */
