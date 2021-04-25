/*
 * led.c
 *
 *  Created on: Jun 14, 2020
 *      Author: snork
 */

#include <led.h>
#include "main.h"

#include <stm32f4xx_hal.h>


void led_init()
{
	// Все настроено халом
}


void led_set(bool state)
{
	// Все наоборот. 0 соответствует включенному леду, потому что он подключен
	// к пину в качестве земли. Пин работает на OpenDrain
	GPIO_PinState pin_state = state ? GPIO_PIN_RESET : GPIO_PIN_SET;

	HAL_GPIO_WritePin(LED_Pin_GPIO_Port, LED_Pin_Pin, pin_state);
}


void led_toggle()
{
	HAL_GPIO_TogglePin(LED_Pin_GPIO_Port, LED_Pin_Pin);
}
