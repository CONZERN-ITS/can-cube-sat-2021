/*
 * adc.h
 *
 *  Created on: May 30, 2020
 *      Author: snork
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include <stdint.h>


typedef enum ldiode_swap_t {
    LDIODE_SWAP_0 = 2,
    LDIODE_SWAP_1 = 1,
    LDIODE_SWAP_2 = 0,
    LDIODE_SWAP_3 = 6,
    LDIODE_SWAP_4 = 5,
    LDIODE_SWAP_5 = 4,
    LDIODE_SWAP_6 = 3,
    LDIODE_SWAP_7 = 9,
    LDIODE_SWAP_8 = 8,
    LDIODE_SWAP_9 = 7,
} ldiode_swap_t;

//! Целевой аналоговый сенсор, с которым требуется работать
typedef enum analog_target_t
{
	//! Интегрированный в stm32 термистр
	ANALOG_TARGET_INTEGRATED_TEMP,
	//! Напряжение на VBAT
	ANALOG_TARGET_VBAT,
	// Фотоидиоды


/*    ANALOG_TARGET_LED_0,
    ANALOG_TARGET_LED_1,
    ANALOG_TARGET_LED_2,
    ANALOG_TARGET_LED_3,
    ANALOG_TARGET_LED_4,
    ANALOG_TARGET_LED_5,
    ANALOG_TARGET_LED_6,
    ANALOG_TARGET_LED_7,
    ANALOG_TARGET_LED_8,
    ANALOG_TARGET_LED_9,*/

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
int analog_get_raw(analog_target_t target, uint16_t oversampling, uint16_t * value);

//! Чтение и расчет сырого значения VDDA
int analog_get_vdda_mv(uint16_t oversampling, uint16_t * value);

#endif /* INC_ADC_H_ */
