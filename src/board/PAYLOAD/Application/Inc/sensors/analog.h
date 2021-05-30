/*
 * adc.h
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include <stdint.h>

//! Номинальное значение VDDA (мВ)
#define ANALOG_VDDA_NOMINAL 3300

//! Целевой аналоговый сенсор, с которым требуется работать
typedef enum analog_target_t
{
	//! NO2 сенсор из MIC-6814
	ANALOG_TARGET_MICS6814_NO2,
	//! NH3 сенсор из MIC-6814
	ANALOG_TARGET_MICS6814_NH3,
	//! CO сенсор из MIC-6814
	ANALOG_TARGET_MICS6814_CO,

	//! O2 сенсор ME202
	ANALOG_TARGET_ME202_O2,

	//! внешний термистр для ДНК
	ANALOG_TARGET_DNA_TEMP,

	//! Интегрированный в stm32 термистр
	ANALOG_TARGET_INTEGRATED_TEMP
} analog_target_t;


//! Инициализация аналоговой подсистемы
int analog_init(void);

//! Реинициализация аналоговой подсистемы
/*! Резет всего до чего дотягиваемся и повторная инициализация */
int analog_restart(void);

//! Чтение сырого значения АЦП для указанного датчика
int analog_get_raw(analog_target_t target, uint16_t * value);

//! Тоже самое что и analog_get_raw, но вместе с сырым значенеим АЦП считает еще и vdda
int analog_get_raw_with_vdda(analog_target_t target, uint16_t * value, uint16_t * vdda);

//! Высчитка напряжения питания VDDA
int analog_get_vdda_mv(uint16_t * value);


#endif /* INC_ADC_H_ */
