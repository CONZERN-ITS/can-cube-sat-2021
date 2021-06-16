#ifndef CLOCK_CONTROL_H_
#define CLOCK_CONTROL_H_


#include <stdint.h>


typedef enum clock_control_active_high_osc_t
{
	CLOCK_CONTROL_HIGH_OSC_HSI,
	CLOCK_CONTROL_HIGH_OSC_HSE,
	CLOCK_CONTROL_HIGH_OSC_INVALID,
} clock_control_active_high_osc_t;


typedef struct clock_control_status_t
{
	clock_control_active_high_osc_t active_high_osc;
	uint32_t css_counter;
} clock_control_status_t;


//! Настройка стандартных (максимальныз) для наших проектов частот на HSI
/*! Функцию можно вызывать вместо кубовской SystemCoreClock_Config.
    Функция запускает PLL на HSI так, что:
    SYSCLK: 168 МГц
    PCLK1: 42 МГц
    PCLK2: 84 МГц

    LSE LSI не настраивает и не трогает
*/
int clock_control__high_clocks_setup_on_hsi(void);

//! Перепрыгиваем системными клоками на PLL@HSE
/*! Переставляем PLL на HSE, чтобы все работало так же как на HSI */
int clock_control__high_clocks_setup_on_hse(void);

//! Запуск HSI/HSE клоков по старту
int clock_control__high_clocks_startup(void);

//! Текущее состояние
int clock_control__high_clock_get_status(clock_control_status_t * status);

#endif /* CLOCK_CONTROL_H_ */
