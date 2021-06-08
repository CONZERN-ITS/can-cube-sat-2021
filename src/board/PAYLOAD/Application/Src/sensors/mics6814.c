#include "sensors/mics6814.h"

#include <math.h>
#include <errno.h>
#include <stdlib.h>

#include <stm32f4xx_hal.h>

#include "time_svc.h"
#include "sensors/analog.h"

/*
	Будем делать вот как. В даташите есть красивые графики,
	показывающие зависимость r/r0 от ppm для разных газов
	Всякие левые газы мы просто не будет учитывать и для каждого сенсора возьмем только "титульный" газ

	Будем искать функцию зависимости ppm(r/r0) через график.
	Обозначим ppm как y, а r/r0 как х. На графиках наоборот, но нам нужна обратная зависимость.

	График линейный в осях log-log а значит описывается выражением log(y) = a*log(x) + b.
	Решаем вот такую систему уравнений относительно a и b
	log(y1) = a*log(x1) + b
	log(y2) = a*log(x2) + b

	Получается:
	a = (log(y1) - log(y2)) / (log(x1) - log(x2))
	b = log(y2) - a * log(x2)

	Теперь у нас есть выражение: log(y) = a*log(x) + b, в котором мы знаем и a и b.
	Экспоненцируем обе части уравнения и получаем
	y = x**a * e**b.

	// Таким подходом определяем зависимости для всех трех сеносоров
*/

// О подключении датчика
/*
 Каждый сенсор датчика подключен по следующей схеме:

 vcc 3.3 +--====--+--====--+--====--+--====-- gnd
         |   r1   |   r2   |   r3   |   rx
         |       /        /         |
         |        |        |        |
         +--------+--------+        +---- к АЦП

  r1, r2 и r3 это известные нам балансирующие резисторы.
  rx - незивестное сопротивление, которое выдает сенсор.
  Поскольку rx может меняться очень сильно, а мы хотим знать его довольно точно,
  мы сделали переменное плечо делителя, с которого снимаем напряжение посредством АЦП.
  Это плечо может состять только из r3, или из r3 + r2 или из r3 + r2 + r1*/


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Сенсор CO (RED)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Коэфициенты пересчёта в ppm
/* Посчитаны по графику в даташите из двух точек:
 * x1 = 0.01, y1 = 1000
 * x2 = 0.4, y2 = 10 */
#define MICS6814_CO_COEFFS_A	(-1.2483927011635698)
#define MICS6814_CO_COEFFS_EXP_B	(3.1857713161420667)

// Значение R0 (сопротивление сенсора в калибровочной атмосфере)
#define MICS6814_CO_R0	(350*1000)
// Значение резистора в верхнем плече делителя сенсора
#define MICS6814_CO_RB_1	(292*1000)
#define MICS6814_CO_RB_2	(292*1000)
#define MICS6814_CO_RB_3	(292*1000)

//FIXME: поменять пины
// Второй порт/пин для управления балансирующими резисторами
#define MICS6814_CO_CTRL_2_PORT		GPIOB
#define MICS6814_CO_CTRL_2_PIN		GPIO_PIN_2

// Первый порт/пин для управления балансирующими резисторами
#define MICS6814_CO_CTRL_1_PORT		GPIOB
#define MICS6814_CO_CTRL_1_PIN		GPIO_PIN_1

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Сенсор NO2 (OX)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Коэфициенты пересчёта в ppm
/* Посчитаны по графику в даташите из двух точек:
 * x1 = 20, y1 = 3
 * x2 = 0.3, y2 = 0.05 */
#define MICS6814_NO2_COEFFS_A	(0.9749124012986611)
#define MICS6814_NO2_COEFFS_EXP_B	(0.16170780328191597)

// Значение R0 (сопротивление сенсора в калибровочной атмосфере)
#define MICS6814_NO2_R0	(4.30*1000)
// Значение резистора в верхнем плече делителя сенсора
#define MICS6814_NO2_RB_1	(4.26*1000)
#define MICS6814_NO2_RB_2	(4.26*1000)
#define MICS6814_NO2_RB_3	(4.26*1000)

// Второй порт/пин для управления балансирующими резисторами
#define MICS6814_NO2_CTRL_2_PORT	GPIOA
#define MICS6814_NO2_CTRL_2_PIN		GPIO_PIN_4

// Первый порт/пин для управления балансирующими резисторами
#define MICS6814_NO2_CTRL_1_PORT	GPIOA
#define MICS6814_NO2_CTRL_1_PIN		GPIO_PIN_5

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Сенсор NH3
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Коэфициенты пересчёта в ppm
/* Посчитаны по графику в даташите из двух точек:
 * x1 = 0.8, y1 = 1
 * x2 = 0.08, y2 = 70*/
#define MICS6814_NH3_COEFFS_A	(-1.845098040014257)
#define MICS6814_NH3_COEFFS_EXP_B	(0.6625086072228469)

// Значение R0 (сопротивление сенсора в калибровочной атмосфере)
#define MICS6814_NH3_R0	(98.30*1000)
// Значение резистора в верхнем плече делителя сенсора
#define MICS6814_NH3_RB_1	(18.78*1000)
#define MICS6814_NH3_RB_2	(18.78*1000)
#define MICS6814_NH3_RB_3	(18.78*1000)

// Второй порт/пин для управления балансирующими резисторами
#define MICS6814_NH3_CTRL_2_PORT	GPIOA
#define MICS6814_NH3_CTRL_2_PIN		GPIO_PIN_6

// Первый порт/пин для управления балансирующими резисторами
#define MICS6814_NH3_CTRL_1_PORT	GPIOA
#define MICS6814_NH3_CTRL_1_PIN		GPIO_PIN_7


//! С каким именно сенсором мы работаем
typedef enum mics6814_sensor_t {
	MICS6814_SENSOR_CO,
	MICS6814_SENSOR_NO2,
	MICS6814_SENSOR_NH3,
} mics6814_sensor_t;


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Управление верхним плечем делителя
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Количество резисторов в верхнем плече резисторного делителя сенсоров
typedef enum mics6814_divider_upper_half_mode_t {
	//! Используются три резистора в плече
	MICS6814_DIVIER_UPPER_HALF_TRIPLE = 3,
	//! Используются два резистора в плече
	MICS6814_DIVIER_UPPER_HALF_DOUBLE = 2,
	//! Используется один резистор в плече
	MICS6814_DIVIER_UPPER_HALF_SINGLE = 1,
} mics6814_divider_upper_half_mode_t;


void mics6814_power_on()
{
	HAL_GPIO_WritePin(MICS_PWR_GPIO_Port, MICS_PWR_Pin, SET);
}


void mics6814_power_off()
{
	HAL_GPIO_WritePin(MICS_PWR_GPIO_Port, MICS_PWR_Pin, RESET);
}


// Перевод пина в input и highz состояние
static void _pin_to_input_mode(GPIO_TypeDef * port, uint32_t pin)
{
	GPIO_InitTypeDef init;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	init.Mode = GPIO_MODE_INPUT;
	init.Pin = pin;

	HAL_GPIO_Init(port, &init);
}


// Перевод пина в output состояние с высоким логическим уровнем
static void _pin_to_high_output(GPIO_TypeDef * port, uint32_t pin)
{
	GPIO_InitTypeDef init;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pin = pin;

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
	HAL_GPIO_Init(port, &init);
}


// Устанавливаем верхнее плечо делителя для указанного сенсора
int _divider_upper_half_set_mode(mics6814_sensor_t target, mics6814_divider_upper_half_mode_t mode)
{
	int error;
	GPIO_TypeDef * port1, * port2;
	uint32_t pin1, pin2;

	switch (target)
	{
	case MICS6814_SENSOR_CO:
		port1 = MICS6814_CO_CTRL_1_PORT;
		pin1 = MICS6814_CO_CTRL_1_PIN;
		port2 = MICS6814_CO_CTRL_2_PORT;
		pin2 = MICS6814_CO_CTRL_2_PIN;
		break;

	case MICS6814_SENSOR_NO2:
		port1 = MICS6814_NO2_CTRL_1_PORT;
		pin1 = MICS6814_NO2_CTRL_1_PIN;
		port2 = MICS6814_NO2_CTRL_2_PORT;
		pin2 = MICS6814_NO2_CTRL_2_PIN;
		break;

	case MICS6814_SENSOR_NH3:
		port1 = MICS6814_NH3_CTRL_1_PORT;
		pin1 = MICS6814_NH3_CTRL_1_PIN;
		port2 = MICS6814_NH3_CTRL_2_PORT;
		pin2 = MICS6814_NH3_CTRL_2_PIN;
		break;

	default:
		error = -ENOSYS;
		break;
	};

	if (0 != error)
		return error;

	switch (mode)
	{
	case MICS6814_DIVIER_UPPER_HALF_TRIPLE:
		_pin_to_input_mode(port1, pin1);
		_pin_to_input_mode(port2, pin2);
		break;

	case MICS6814_DIVIER_UPPER_HALF_DOUBLE:
		_pin_to_input_mode(port1, pin1);
		_pin_to_high_output(port2, pin2);
		break;

	case MICS6814_DIVIER_UPPER_HALF_SINGLE:
		_pin_to_high_output(port1, pin1);
		_pin_to_high_output(port2, pin2);
		break;

	default:
		error = -EINVAL;
	}

	if (0 != error)
		return error;

	return 0;
}


//! Итоговое сопротивление делителя
static int _divider_rb(mics6814_sensor_t target, mics6814_divider_upper_half_mode_t mode, float * value)
{
	float r1, r2, r3;
	switch (target)
	{
	case MICS6814_SENSOR_CO:
		r1 = MICS6814_CO_RB_1;
		r2 = MICS6814_CO_RB_2;
		r3 = MICS6814_CO_RB_3;
		break;

	case MICS6814_SENSOR_NO2:
		r1 = MICS6814_NO2_RB_1;
		r2 = MICS6814_NO2_RB_2;
		r3 = MICS6814_NO2_RB_3;
		break;

	case MICS6814_SENSOR_NH3:
		r1 = MICS6814_NH3_RB_1;
		r2 = MICS6814_NH3_RB_2;
		r3 = MICS6814_NH3_RB_3;
		break;

	default:
		return -EINVAL;
	}

	switch (mode)
	{
	case MICS6814_DIVIER_UPPER_HALF_SINGLE:
		*value = r3;
		break;

	case MICS6814_DIVIER_UPPER_HALF_DOUBLE:
		*value = r3 + r2;
		break;

	case MICS6814_DIVIER_UPPER_HALF_TRIPLE:
		*value = r3 + r2 + r1;
		break;

	default:
		return -EINVAL;
	};

	return 0;
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Работа с сенсором по-крупному
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


void mics6814_init()
{
	// Нужно настроить GPIO для упрваления плечами
	// но они правильно настроены с резета (в in)
	// поэтому мы их не трогаем тоже
	// Только включим клоки (хотя они и так должны быть включены)
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// Настриваем все резисторы
	_divider_upper_half_set_mode(MICS6814_SENSOR_CO,  MICS6814_DIVIER_UPPER_HALF_TRIPLE);
	_divider_upper_half_set_mode(MICS6814_SENSOR_NO2, MICS6814_DIVIER_UPPER_HALF_TRIPLE);
	_divider_upper_half_set_mode(MICS6814_SENSOR_NH3, MICS6814_DIVIER_UPPER_HALF_TRIPLE);
}


// Делает замер для одного из сенсоров датчика
/* \arg target задает нужный сенсор
   \arg dr возвращает сырое отношение сопротивлений сенсора
   \arg conc возвращает концентрацию __титульного__ газа сенсора */
static int _read_one(
		mics6814_sensor_t target, uint16_t * adc_raw_, uint8_t * rb_mode, float * conc
)
{
	int error = 0;

	float r0;
	float a, exp_b;
	analog_target_t analog_target;
	const int16_t adc_max = 0x0FFF;

	// Определяемся с параметрами сенсора
	switch (target)
	{
	case MICS6814_SENSOR_CO:
		r0 = MICS6814_CO_R0;
		a  = MICS6814_CO_COEFFS_A;
		exp_b  = MICS6814_CO_COEFFS_EXP_B;
		analog_target = ANALOG_TARGET_MICS6814_CO;
		break;

	case MICS6814_SENSOR_NO2:
		r0 = MICS6814_NO2_R0;
		a  = MICS6814_NO2_COEFFS_A;
		exp_b  = MICS6814_NO2_COEFFS_EXP_B;
		analog_target = ANALOG_TARGET_MICS6814_NO2;
		break;

	case MICS6814_SENSOR_NH3:
		r0 = MICS6814_NH3_R0;
		a  = MICS6814_NH3_COEFFS_A;
		exp_b  = MICS6814_NH3_COEFFS_EXP_B;
		analog_target = ANALOG_TARGET_MICS6814_NH3;
		break;

	default:
		error = -EINVAL;
		break;
	}

	// делаем замер для каждого из доступных плеч делителя
	// И сразу же выбираем лучшее плечо, которое дало
	// результат ближе всего к центру шкалы АЦП
	mics6814_divider_upper_half_mode_t modes[] = {
			MICS6814_DIVIER_UPPER_HALF_SINGLE,
			MICS6814_DIVIER_UPPER_HALF_DOUBLE,
			MICS6814_DIVIER_UPPER_HALF_TRIPLE,
	};
	uint16_t adc_raw[sizeof(modes)/sizeof(*modes)];
	int32_t best_raw_diff = INT32_MAX;
	size_t best_raw_idx = 0;
	for(size_t i = 0; i < sizeof(modes)/sizeof(*modes); i++)
	{
		mics6814_divider_upper_half_mode_t mode = modes[i];
		error = _divider_upper_half_set_mode(target, mode);
		if (0 != error)
			return error;

		const int oversampling = 50;
		error = analog_get_raw(analog_target, oversampling, &adc_raw[i]);
		if (0 != error)
			return error;

		int32_t diff = abs(adc_raw[i] - adc_max / 2);
		if (diff < best_raw_diff)
		{
			best_raw_diff = diff;
			best_raw_idx = i;
		}
	}

	// Считаем концентрацию по показаниям лучшего плеча
	mics6814_divider_upper_half_mode_t best_mode = modes[best_raw_idx];
	uint16_t best_raw = adc_raw[best_raw_idx];

	float rb;
	error = _divider_rb(target, best_mode, &rb);
	if (0 != error)
		return error;

	// Считаем сопротивление сенсора
	float dv = (float)best_raw/adc_max;
	float rx = rb * dv/(1 - dv);

	// Пересчитываем в концентрацию
	float dr = rx/r0;
	*conc = pow(dr, a) * exp_b;

	// Закидываем сырые значения из которых считали
	memcpy(adc_raw_, adc_raw, sizeof(adc_raw));
	*rb_mode = (uint8_t)best_mode;
	return 0;
}


int mics6814_read(mavlink_pld_mics_6814_data_t * msg)
{
	// Берем время
	struct timeval tv;
	time_svc_gettimeofday(&tv);

	msg->time_s = tv.tv_sec;
	msg->time_us = tv.tv_usec;
	msg->time_steady = HAL_GetTick();

	int error;

	// Теперь опрашиваем все сенсоры
	uint16_t adc_raw[3];
	uint8_t rb_mode;
	float concentration;


	error = _read_one(MICS6814_SENSOR_CO, adc_raw, &rb_mode, &concentration);
	if (0 != error)
		return error;

	memcpy(msg->red_sensor_raw, adc_raw, sizeof(adc_raw));
	msg->red_rb = rb_mode;
	msg->co_conc = concentration;


	error = _read_one(MICS6814_SENSOR_NO2, adc_raw, &rb_mode, &concentration);
	if (0 != error)
		return error;

	memcpy(msg->ox_sensor_raw, adc_raw, sizeof(adc_raw));
	msg->ox_rb = rb_mode;
	msg->no2_conc = concentration;


	error = _read_one(MICS6814_SENSOR_NH3, adc_raw, &rb_mode, &concentration);
	if (0 != error)
		return error;

	memcpy(msg->nh3_sensor_raw, adc_raw, sizeof(adc_raw));
	msg->nh3_rb = rb_mode;
	msg->nh3_conc = concentration;

	return 0;
}
