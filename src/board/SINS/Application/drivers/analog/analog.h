/*
 * adc.h
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include <stdint.h>


// Напряжение со встроенного термистра при 25 градусах (в милливольта)
#define INTERNAL_TEMP_V25 (760.0f)
// Коэффициент k внутренного термистра (мВ/C
#define INTERNAL_TEMP_AVG_SLOPE (2.5f)


//! Целевой аналоговый сенсор, с которым требуется работать
typedef enum analog_target_t
{
	//! Интегрированный в stm32 термистр
	ANALOG_TARGET_INTEGRATED_TEMP,
	// Фотоидиоды
	ANALOG_TARGET_LED_0,
	ANALOG_TARGET_LED_1,
	ANALOG_TARGET_LED_2,
	ANALOG_TARGET_LED_3,
	ANALOG_TARGET_LED_4,
	ANALOG_TARGET_LED_5,
	ANALOG_TARGET_LED_6,
	ANALOG_TARGET_LED_7,
	ANALOG_TARGET_LED_8,
	ANALOG_TARGET_LED_9,
} analog_target_t;


//! Инициализация аналоговой подсистемы
int analog_init(void);

//! Реинициализация аналоговой подсистемы
/*! Резет всего до чего дотягиваемся и повторная инициализация */
int analog_restart(void);

//! Чтение сырого значения АЦП для указанного датчика
int analog_get_raw(analog_target_t target, uint16_t * value);


#endif /* INC_ADC_H_ */