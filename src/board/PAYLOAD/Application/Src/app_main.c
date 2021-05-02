#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <led.h>
#include <sensors/its_ms5611.h>

#include <stm32f4xx_hal.h>

#include "Inc/its-i2c-link.h"

#include "app_main.h"
#include "main.h"
#include "mavlink_main.h"
#include "util.h"
#include "time_svc.h"

#include "sensors/its_bme280.h"
#include "sensors/analog.h"
#include "sensors/me2o2.h"
#include "sensors/mics6814.h"
#include "sensors/integrated.h"

//! Длина одного такта в мс
#define TICK_LEN_MS (200)
//! Сколько времени диод будет гореть а не мигать после рестарта (в мс)
#define LED_RESTART_LOCK (1500)

//! Периодичность выдачи BME пакета (в тактах)
#define PACKET_PERIOD_BME (500)//(5)
#define PACKET_OFFSET_BME (0)
//! Периодичность выдачи пакета MS5611
#define PACKET_PERIOD_MS5611 (5)
#define PACKET_OFFSET_MS5611 (0)
//! Периодичность выдачи ME202 пакета (в тактах)
#define PACKET_PERIOD_ME2O2 (500)//(5)
#define PACKET_OFFSET_ME2O2 (1)
//! Периодичность выдачи MICS6814 пакета (в тактах)
#define PACKET_PERIOD_MICS6814 (500)//(5)
#define PACKET_OFFSET_MICS6814 (2)
//! Периодичность выдачи данных со встроенных сенсоров (в тактах)
#define PACKET_PERIOD_INTEGRATED (500)//(5)
#define PACKET_OFFSET_INTEGRATED (3)
//! Периодичность выдачи собственной статистики (в тактах)
#define PACKET_PERIOD_OWN_STATS (500)//(15)
#define PACKET_OFFSET_OWN_STATS (0)
//! Периодичность выдачи its-link статистики (в тактах)
#define PACKET_PERIOD_ITS_LINK_STATS (500)//(15)
#define PACKET_OFFSET_ITS_LINK_STATS (7)


//! Статистика об ошибках этого модуля
typedef struct its_pld_status_t
{
	//! Код последней ошибки работы с bme внутри герметичного корпуса
	int32_t int_bme_last_error;
	//! Количество ошибок при работе с bme внутри герметичного корпуса
	uint16_t int_bme_error_counter;

	//! Код последней ошибки работы с bme снаружи герметичного корпуса
	int32_t ext_bme_last_error;
	//! Количество ошибок при работе с bme снаружи герметичного корпуса
	uint16_t ext_bme_error_counter;

	//! Код последней ошибки при работе с АЦП
	int32_t adc_last_error;
	//! Количество ошибок при работе с АЦП
	uint16_t adc_error_counter;

	//! Количество перезагрузок модуля
	uint16_t resets_count;
	//! Причина последней перезагрузки
	uint16_t reset_cause;
} its_pld_status_t;


static its_pld_status_t _status = {0};


extern I2C_HandleTypeDef hi2c2;
extern IWDG_HandleTypeDef hiwdg;

typedef int (*initializer_t)(void);

//! Собираем собственную статистику в мав пакет
static void _collect_own_stats(mavlink_pld_stats_t * msg);
//! Пакуем i2c-link статистику в мав пакет
static void _collect_i2c_link_stats(mavlink_i2c_link_stats_t * msg);

//! Анализ кода возвращенного внутреннему bme операцией
static int _int_bme_op_analysis(int rc);
//! Убеждаемся в том, что внутренний БМЕ функционален. Пытаемся его рестартнуть если нет
static int _int_bme_restart_if_need_so(void);

//! Анализ кода возвращенного внешнему bme операцией
static int _ext_bme_op_analysis(int rc);
//! Убеждаемся в том, что внешний БМЕ функционален. Пытаемся его рестартнуть если нет
static int _ext_bme_restart_if_need_so(void);

//! Анализ кода возвращенного аналоговой операцией
static int _analog_op_analysis(int rc);
//! Убеждаемся в том, что АЦП функционален. Пытаемся его рестартнуть если нет
static int _analog_restart_if_need_so(void);

//! Обработка вхоядщие пакетов
static void _process_input_packets(void);

//! Извлекает причину резета из системных регистров
/*! Возвращает значение - битовую комбинацию флагов  */
static uint16_t _fetch_reset_cause(void);

// TODO:
/* калибрануть все и вся
   проверить как работает рестарт АЦП
 */

int app_main()
{
	// Грузим из бэкап регистров количество рестартов, которое с нами случилось
	_status.resets_count = LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR1);
	_status.resets_count += 1;
	LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR1, _status.resets_count);

	// Смотрим причину последнего резета
	_status.reset_cause = _fetch_reset_cause();

	// Включаем все
	led_init();
	time_svc_init();
	its_i2c_link_start(&hi2c2);

	_int_bme_op_analysis(its_bme280_reinit(ITS_BME_LOCATION_EXTERNAL));
	_int_bme_op_analysis(its_bme280_reinit(ITS_BME_LOCATION_INTERNAL));
	int_ms5611_reinit(ITS_MS_EXTERNAL);
	int_ms5611_reinit(ITS_MS_INTERNAL);
	_analog_op_analysis(analog_init());
	mics6814_init();

	// После перезагрузки будем аж пол секунды светить лампочкой
	uint32_t tick_begin = HAL_GetTick();
	uint32_t tick_led_unlock = tick_begin + LED_RESTART_LOCK;

	led_set(true);

	int64_t tock = 0;
	while(1)
	{
		// Сбрасываем вотчдог
		// UPD делаем это не тут, а в mavlink_main при успешной отправке сообщения
		// Сдесь сбрасываем только если мавлинк отключен для отладки
#ifndef PROCESS_TO_ITSLINK
		HAL_IWDG_Refresh(&hiwdg);
#endif
		// Запоминаем когда этот такт начался
		uint32_t tock_start_tick = HAL_GetTick();
		// Планируем начало следующего
		uint32_t next_tock_start_tick = tock_start_tick + TICK_LEN_MS;

		// В начале такта включаем диод (если после загрузки прошло нужное время)
		// И запоминаем не раньше какого времени нужно выключить светодиод
		uint32_t led_off_tick;
		if (HAL_GetTick() >= tick_led_unlock)
		{
			led_set(true);
			led_off_tick = tock_start_tick + 10;
		}
		else
		{
			// Если еще не пора - то и выключать не будем диод в конце такта
			led_off_tick = tick_led_unlock;
		}

		// Проверяем входящие пакеты
		_process_input_packets();

		if (tock % PACKET_PERIOD_BME == PACKET_OFFSET_BME)
		{
			if (0 == _int_bme_restart_if_need_so())
			{
				mavlink_pld_int_bme280_data_t bme_msg = {0};
				int rc = _int_bme_op_analysis(its_bme280_read(ITS_BME_LOCATION_INTERNAL, &bme_msg));
				if (0 == rc)
					mav_main_process_int_bme_message(&bme_msg);
			}
			if (0 == _ext_bme_restart_if_need_so())
			{
				mavlink_pld_int_bme280_data_t bme_msg = {0};
				int rc = _int_bme_op_analysis(its_bme280_read(ITS_BME_LOCATION_EXTERNAL, &bme_msg));
				if (0 == rc)
					mav_main_process_int_bme_message(&bme_msg);
			}
		}


		if (tock % PACKET_PERIOD_MS5611 == PACKET_OFFSET_MICS6814)
		{
			mavlink_pld_int_ms5611_data_t data;
			// int rc = int_ms5611_read_and_calculate(&data);
			int rc = 0;
			printf("rc = %d\n", rc);
			mav_main_process_ms5611_message(&data);
		}


		if (tock % PACKET_PERIOD_ME2O2 == PACKET_OFFSET_ME2O2)
		{
			if (0 == _analog_restart_if_need_so())
			{
				mavlink_pld_me2o2_data_t me2o2_msg = {0};
				int rc = _analog_op_analysis(me2o2_read(&me2o2_msg));
				if (0 == rc)
					mav_main_process_me2o2_message(&me2o2_msg);
			}
		}


		if (tock % PACKET_PERIOD_MICS6814 == PACKET_OFFSET_MICS6814)
		{
			if (0 == _analog_restart_if_need_so())
			{
				mavlink_pld_mics_6814_data_t mics_msg = {0};
				if (0 == _analog_op_analysis(mics6814_read(&mics_msg)))
					mav_main_process_mics_message(&mics_msg);
			}
		}


		if (tock % PACKET_PERIOD_INTEGRATED == PACKET_OFFSET_INTEGRATED)
		{
			if (0 == _analog_restart_if_need_so())
			{
				mavlink_own_temp_t own_temp_msg = {0};
				if (0 == _analog_op_analysis(integrated_read(&own_temp_msg)))
					mav_main_process_owntemp_message(&own_temp_msg);
			}
		}


		if (tock % PACKET_PERIOD_OWN_STATS == PACKET_OFFSET_OWN_STATS)
		{
			mavlink_pld_stats_t pld_stats_msg = {0};
			_collect_own_stats(&pld_stats_msg);
			mav_main_process_own_stats(&pld_stats_msg);
		}


		if (tock % PACKET_PERIOD_ITS_LINK_STATS == PACKET_OFFSET_ITS_LINK_STATS)
		{
			mavlink_i2c_link_stats_t i2c_stats_msg = {0};
			_collect_i2c_link_stats(&i2c_stats_msg);
			mav_main_process_i2c_link_stats(&i2c_stats_msg);
		}


		// Ждем начала следующего такта
		uint32_t now;
		bool led_done  = false;
		do {
			now = HAL_GetTick();
			// Выключаем диод если пора
			if (!led_done && now >= led_off_tick)
			{
				led_set(false);
				led_done  = true;
			}

		} while(now < next_tock_start_tick);
		// К следующему такту
		tock++;
	}

	return 0;
}


static void _collect_own_stats(mavlink_pld_stats_t * msg)
{
	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;
	msg->adc_error_counter = _status.adc_error_counter;
	msg->adc_last_error = _status.adc_last_error;
	msg->int_bme_error_counter = _status.int_bme_error_counter;
	msg->int_bme_last_error = _status.int_bme_last_error;
	msg->ext_bme_error_counter = _status.ext_bme_error_counter;
	msg->ext_bme_last_error = _status.ext_bme_last_error;
	msg->resets_count = _status.resets_count;
	msg->reset_cause = _status.reset_cause;
}


static void _collect_i2c_link_stats(mavlink_i2c_link_stats_t * msg)
{
	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;

	its_i2c_link_stats_t statsbuf;
	its_i2c_link_stats(&statsbuf);

	msg->rx_done_cnt = statsbuf.rx_done_cnt;
	msg->rx_dropped_cnt = statsbuf.rx_dropped_cnt;
	msg->rx_error_cnt = statsbuf.rx_error_cnt;
	msg->tx_done_cnt = statsbuf.tx_done_cnt;
	msg->tx_zeroes_cnt = statsbuf.tx_zeroes_cnt;
	msg->tx_error_cnt = statsbuf.tx_error_cnt;
	msg->tx_overrun_cnt = statsbuf.tx_overrun_cnt;
	msg->restarts_cnt = statsbuf.restarts_cnt;
	msg->listen_done_cnt = statsbuf.listen_done_cnt;
	msg->last_error = statsbuf.last_error;
}


static void _process_input_packets()
{
	mavlink_message_t input_msg;
	int rc = mav_main_get_packet(&input_msg);
	if (0 == rc)
	{
		// Обрабытываем. Пока только для службы времени
		time_svc_on_mav_message(&input_msg);
	}
}


static uint16_t _fetch_reset_cause(void)
{
	int rc = 0;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
		rc |= MCU_RESET_PIN;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
		rc |= MCU_RESET_POR;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
		rc |= MCU_RESET_SW;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
		rc |= MCU_RESET_WATCHDOG;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
		rc |= MCU_RESET_WATCHDOG2;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
		rc |= MCU_RESET_LOWPOWER;

	__HAL_RCC_CLEAR_RESET_FLAGS();
	return rc;
}


static int _int_bme_op_analysis(int rc)
{
	_status.int_bme_last_error = rc;
	if (rc)
		_status.int_bme_error_counter++;

	return _status.int_bme_last_error;
}


static int _ext_bme_op_analysis(int rc)
{
	_status.ext_bme_last_error = rc;
	if (rc)
		_status.ext_bme_error_counter++;

	return _status.ext_bme_last_error;
}


static int _int_bme_restart_if_need_so(void)
{
	if (_status.int_bme_last_error)
		_int_bme_op_analysis(its_bme280_reinit(ITS_BME_LOCATION_INTERNAL));

	return _status.int_bme_last_error;
}


static int _ext_bme_restart_if_need_so(void)
{
	if (_status.ext_bme_last_error)
		_ext_bme_op_analysis(its_bme280_reinit(ITS_BME_LOCATION_INTERNAL));

	return _status.ext_bme_last_error;
}


static int _analog_op_analysis(int rc)
{
	_status.adc_last_error = rc;
	if (rc)
		_status.adc_error_counter++;

	return _status.adc_last_error;
}


static int _analog_restart_if_need_so(void)
{
	if (_status.adc_last_error)
		_analog_op_analysis(analog_restart());

	return _status.adc_last_error;
}

