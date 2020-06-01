/*
 * adc.h
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include <stdint.h>

//! Целевой аналоговый сенсор, с которым требуется работать
typedef enum its_pld_analog_target_t
{
	//! NO2 сенсор из MIC-6814
	ITS_PLD_ANALOG_TARGET_MICS6814_NO2,
	//! NH3 сенсор из MIC-6814
	ITS_PLD_ANALOG_TARGET_MICS6814_NH3,
	//! CO сенсор из MIC-6814
	ITS_PLD_ANALOG_TARGET_MICS6814_CO,

	//! O2 сенсор ME202
	ITS_PLD_ANALOG_TARGET_ME202_O2,

	//! Интегрированный в stm32 термистр
	ITS_PLD_ANALOG_TARGET_INTEGRATED_TEMP
} its_pld_analog_target_t;


//! Инициализация аналоговой подсистемы
int its_pld_analog_init(void);

//! Чтение сырого значения АЦП для указанного датчика
int its_pld_analog_get_raw(its_pld_analog_target_t target, uint16_t * value);

//! Чтение напряжение с АЦП в милливольтах
/*! Довольно тупая функция, предполагающая что мы питаемся ровно от 3.3 вольта */
int its_pld_analog_get_mv(its_pld_analog_target_t target, float * value);


#endif /* INC_ADC_H_ */
