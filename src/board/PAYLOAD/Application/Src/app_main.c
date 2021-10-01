#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <led.h>
#include <sensors/its_ms5611.h>

#include <stm32f4xx_hal.h>

#include "its-i2c-link.h"

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
#include "sensors/dna_temp.h"
#include "sensors/dosim.h"
#include "its_ccompressor.h"

#include "commissar.h"

#define ITS_PLD_COMPRESSOR_TESTS

//! БМЕ принципиально не показывает давление ниже 30ы 0000
//! Поэтому с этого момента мы его показания для управления компрессором не используем
#define BME_PRESSURE_THRESHOLD_LOW (31*1000) // в даташите 30*1000
#define BME_PRESSURE_THRESHOLD_HI (108*1000) // в даташите 110*1000

//! Длина одного такта в мс
#define TICK_LEN_MS (200)
//! Сколько времени диод будет гореть а не мигать после рестарта (в мс)
#define LED_RESTART_LOCK (1500)

//! Периодичность выдачи BME пакета (в тактах)
#define PACKET_PERIOD_BME (5)
#define PACKET_OFFSET_BME (0)
//! Периодичность выдачи пакета MS5611
#define PACKET_PERIOD_MS5611 (5)
#define PACKET_OFFSET_MS5611 (1)
//! Периодичность выдачи ME202 пакета (в тактах)
#define PACKET_PERIOD_ME2O2 (5)
#define PACKET_OFFSET_ME2O2 (2)
//! Периодичность выдачи MICS6814 пакета (в тактах)
#define PACKET_PERIOD_MICS6814 (5)
#define PACKET_OFFSET_MICS6814 (3)
//! Периодичность выдачи данных со встроенных сенсоров (в тактах)
#define PACKET_PERIOD_INTEGRATED (5)
#define PACKET_OFFSET_INTEGRATED (4)
// ! Периодичность выдачи последних данных с дозиметра (в тактах)
#define PACKET_PERIOD_DOSIM_MOMENTARY (5)
#define PACKET_OFFSET_DOSIM_MOMENTARY (0)
// ! Периодичность выдачи накопленных данных с дозиметра (в тактах)
#define PACKET_PERIOD_DOSIM_WINDOWED (5)
#define PACKET_OFFSET_DOSIM_WINDOWED (0)
//! Периодичность выдачи данных с ДНК (в тактах)
#define PACKET_PERIOD_DNA (5)
#define PACKET_OFFSET_DNA (1)
//! Периодичность выдачи собственной статистики (в тактах)
#define PACKET_PERIOD_OWN_STATS (15)
#define PACKET_OFFSET_OWN_STATS (7)
//! Периодичность выдачи its-link статистики (в тактах)
#define PACKET_PERIOD_ITS_LINK_STATS (15)
#define PACKET_OFFSET_ITS_LINK_STATS (8)
//! Периодичность выдачи информации о состоянии управления компрессором
#define PACKET_PERIOD_CCONTROL (5)
#define PACKET_OFFSET_CCONTROL (2)
//! Периодичность выдачи рапортов коммиссара
#define PACKET_PERIOD_COMMISSAR_REPORT (25)
#define PACKET_OFFSET_COMMISSAR_REPORT (10)


//! Статистика об ошибках этого модуля
typedef struct its_pld_status_t
{
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

//! Обработка вхоядщие пакетов
static void _process_input_packets(void);

//! Извлекает причину резета из системных регистров
/*! Возвращает значение - битовую комбинацию флагов  */
static uint16_t _fetch_reset_cause(void);


int app_main()
{
	// Грузим из бэкап регистров количество рестартов, которое с нами случилось
	_status.resets_count = LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR1);
	_status.resets_count += 1;
	LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR1, _status.resets_count);

	// Смотрим причину последнего резета
	_status.reset_cause = _fetch_reset_cause();

	int HSE_start_time = 0;
	//Если едем на HSE перезапускаться не будем
	if (RCC_PLLSOURCE_HSI == __HAL_RCC_GET_PLL_OSCSOURCE())
	{
		//Будем пытаться запускаться на HSE каждые 10 минут
		HSE_start_time = HAL_GetTick();

	}

	// Включаем все
	led_init();
	time_svc_init();
	its_i2c_link_start();

	its_bme280_init(ITS_BME_LOCATION_EXTERNAL);
	its_bme280_init(ITS_BME_LOCATION_INTERNAL);
	its_ms5611_init(ITS_MS_EXTERNAL);
	its_ms5611_init(ITS_MS_INTERNAL);
	analog_init();
	mics6814_init();
	dosim_init();
	commissar_init();
	its_ccontrol_init();
	dna_control_init();
	// После перезагрузки будем аж пол секунды светить лампочкой
	uint32_t tick_begin = HAL_GetTick();
	uint32_t tick_led_unlock = tick_begin + LED_RESTART_LOCK;

	led_set(true);

	int64_t tock = 0;
	while(1)
	{
#ifdef ITS_PLD_COMPRESSOR_TESTS
		// грязный хак для теста компрессора
		const uint32_t compressor_period = 30*1000;
		const uint32_t compressor_on_time = 20*1000;
		const uint32_t valve_close_time = 25*1000;

		static uint32_t compressor_power_on = 0 + compressor_period;
		static uint32_t compressor_power_off = 0;
		static uint32_t valve_power_off = 0;
		static bool compressor_on = false;
		static bool valve_closed = false;

		if (compressor_on && HAL_GetTick() >= compressor_power_off)
		{
			compressor_power_on += compressor_period;
			its_ccontrol_pump_override(false);
			compressor_on = false;
		}
		if (!compressor_on && HAL_GetTick() >= compressor_power_on)
		{
			compressor_power_off = compressor_power_on + compressor_on_time;
			its_ccontrol_pump_override(true);
			compressor_on = true;

			valve_power_off = compressor_power_on + valve_close_time;
			its_ccontrol_valve_override(true);
			valve_closed = true;
		}
		if (valve_closed && HAL_GetTick() >= valve_power_off)
		{
			its_ccontrol_valve_override(false);
			valve_closed = false;
		}
		// Конец грязного хака
#endif // ITS_PLD_COMPRESSOR_TESTS

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
		// Управляем температурой ДНК
		dna_control_work();
		// Управляем компрессором
		its_ccontrol_work();
		// Аккамулируем значения дозиметра
		dosim_work();

		if (tock % PACKET_PERIOD_BME == PACKET_OFFSET_BME)
		{
			mavlink_pld_bme280_data_t bme_msg = {0};
			int rc = its_bme280_read(ITS_BME_LOCATION_INTERNAL, &bme_msg);
			commissar_accept_report(COMMISSAR_SUB_BME280_INT, rc);
			if (0 == rc)
			{
				if (bme_msg.pressure > BME_PRESSURE_THRESHOLD_LOW && bme_msg.pressure < BME_PRESSURE_THRESHOLD_HI)
					its_ccontrol_update_inner_pressure(bme_msg.pressure);

				mav_main_process_bme_message(&bme_msg, PLD_LOC_INTERNAL);
			}

			rc = its_bme280_read(ITS_BME_LOCATION_EXTERNAL, &bme_msg);
			commissar_accept_report(COMMISSAR_SUB_BME280_EXT, rc);
			if (0 == rc)
			{
				if (bme_msg.pressure > BME_PRESSURE_THRESHOLD_LOW && bme_msg.pressure < BME_PRESSURE_THRESHOLD_HI)
					its_ccontrol_update_altitude(bme_msg.altitude, 0);

				mav_main_process_bme_message(&bme_msg, PLD_LOC_EXTERNAL);
			}
		}

		if (tock % PACKET_PERIOD_MS5611 == PACKET_OFFSET_MS5611)
		{
			mavlink_pld_ms5611_data_t data = {0};
			int rc = its_ms5611_read_and_calculate(ITS_MS_EXTERNAL, &data);
			commissar_accept_report(COMMISSAR_SUB_MS5611_EXT, rc);
			if (rc == 0)
			{
				its_ccontrol_update_altitude(data.altitude, 1);
				mav_main_process_ms5611_message(&data, PLD_LOC_EXTERNAL);
			}

			rc = its_ms5611_read_and_calculate(ITS_MS_INTERNAL, &data);
			commissar_accept_report(COMMISSAR_SUB_MS5611_INT, rc);
			if (rc == 0)
			{
				its_ccontrol_update_inner_pressure(data.pressure);
				mav_main_process_ms5611_message(&data, PLD_LOC_INTERNAL);
			}
		}


		if (tock % PACKET_PERIOD_ME2O2 == PACKET_OFFSET_ME2O2)
		{
			mavlink_pld_me2o2_data_t me2o2_msg = {0};
			int rc = me2o2_read(&me2o2_msg);
			if (0 == rc)
				mav_main_process_me2o2_message(&me2o2_msg);
		}


		if (tock % PACKET_PERIOD_MICS6814 == PACKET_OFFSET_MICS6814)
		{
			mavlink_pld_mics_6814_data_t mics_msg = {0};
			int rc = mics6814_read(&mics_msg);
			if (0 == rc)
				mav_main_process_mics_message(&mics_msg);
		}


		if (tock % PACKET_PERIOD_INTEGRATED == PACKET_OFFSET_INTEGRATED)
		{
			mavlink_own_temp_t own_temp_msg = {0};
			int rc = integrated_read(&own_temp_msg);
			if (0 == rc)
				mav_main_process_owntemp_message(&own_temp_msg);
		}


		if (tock % PACKET_PERIOD_DNA == PACKET_OFFSET_DNA)
		{
			mavlink_pld_dna_data_t pld_dna_msg = {0};
			int rc = dna_control_get_status(&pld_dna_msg);
			if (0 == rc)
				mav_main_process_dna_message(&pld_dna_msg);
		}

		if (tock % PACKET_PERIOD_DOSIM_MOMENTARY == PACKET_OFFSET_DOSIM_MOMENTARY)
		{
			mavlink_pld_dosim_data_t pld_dosim_msg = {0};
			dosim_read_momentary(&pld_dosim_msg);
			mav_main_process_dosim_message(&pld_dosim_msg, 0);
		}

		if (tock % PACKET_PERIOD_DOSIM_WINDOWED == PACKET_OFFSET_DOSIM_WINDOWED)
		{
			mavlink_pld_dosim_data_t pld_dosim_msg = {0};
			dosim_read_windowed(&pld_dosim_msg);
			mav_main_process_dosim_message(&pld_dosim_msg, 1);
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

		if (tock % PACKET_PERIOD_CCONTROL == PACKET_OFFSET_CCONTROL) {
			mavlink_pld_compressor_data_t msg;
			its_ccontrol_get_state(&msg);
			mav_main_process_ccompressor_state(&msg);
		}

		if (tock % PACKET_PERIOD_COMMISSAR_REPORT == PACKET_OFFSET_COMMISSAR_REPORT)
		{
			mavlink_commissar_report_t report;
			uint8_t component_id;
			commissar_provide_report(&component_id, &report);
			mav_main_process_commissar_report(component_id, &report);
		}

		// В конце такта работает комиссар
		commissar_work();

		if (RCC_PLLSOURCE_HSI == __HAL_RCC_GET_PLL_OSCSOURCE())
		{
			int HSE_stop_time = HAL_GetTick();

			if ((HSE_stop_time - HSE_start_time) > 10 * 60 * 1000)
				HAL_NVIC_SystemReset();
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
	msg->time_steady = HAL_GetTick();

	msg->resets_count = _status.resets_count;
	msg->reset_cause = _status.reset_cause;

	if (RCC_PLLSOURCE_HSI == __HAL_RCC_GET_PLL_OSCSOURCE())
		msg->active_oscillator = ACTIVE_OSCILLATOR_HSI;
	else
		msg->active_oscillator = ACTIVE_OSCILLATOR_HSE;


	msg->time_base = time_svc_get_time_base();

	time_svc_stats_t time_svc_stats;
	time_svc_get_stats(&time_svc_stats);
	msg->time_syncs_attempted = time_svc_stats.syncs_attempted;
	msg->time_syncs_performed = time_svc_stats.syncs_performed;
	msg->last_time_sync_time_steady = time_svc_stats.last_sync_time_steady;
	msg->last_time_sync_delta_s = time_svc_stats.last_sync_delta_s;
	msg->last_time_sync_delta_us = time_svc_stats.last_sync_delta_us;
}


static void _collect_i2c_link_stats(mavlink_i2c_link_stats_t * msg)
{
	struct timeval tmv;
	time_svc_gettimeofday(&tmv);
	msg->time_s = tmv.tv_sec;
	msg->time_us = tmv.tv_usec;
	msg->time_steady = HAL_GetTick();

	its_i2c_link_stats_t stats;
	its_i2c_link_stats(&stats);

	msg->rx_packet_start_cnt = stats.rx_packet_start_cnt;
	msg->rx_packet_done_cnt = stats.rx_packet_done_cnt;
	msg->rx_cmds_start_cnt = stats.rx_cmds_start_cnt;
	msg->rx_cmds_done_cnt = stats.rx_cmds_done_cnt;
	msg->rx_drops_start_cnt = stats.rx_drops_start_cnt;
	msg->rx_drops_done_cnt = stats.rx_drops_done_cnt;
	msg->tx_psize_start_cnt = stats.tx_psize_start_cnt;
	msg->tx_psize_done_cnt = stats.tx_psize_done_cnt;
	msg->tx_packet_start_cnt = stats.tx_packet_start_cnt;
	msg->tx_packet_done_cnt = stats.tx_packet_done_cnt;
	msg->tx_zeroes_start_cnt = stats.tx_zeroes_start_cnt;
	msg->tx_zeroes_done_cnt = stats.tx_zeroes_done_cnt;
	msg->tx_empty_buffer_cnt = stats.tx_empty_buffer_cnt;
	msg->tx_overruns_cnt = stats.tx_overruns_cnt;
	msg->cmds_get_size_cnt = stats.cmds_get_size_cnt;
	msg->cmds_get_packet_cnt = stats.cmds_get_packet_cnt;
	msg->cmds_set_packet_cnt = stats.cmds_set_packet_cnt;
	msg->cmds_invalid_cnt = stats.cmds_invalid_cnt;
	msg->restarts_cnt = stats.restarts_cnt;
	msg->berr_cnt = stats.berr_cnt;
	msg->arlo_cnt = stats.arlo_cnt;
	msg->ovf_cnt = stats.ovf_cnt;
	msg->af_cnt = stats.af_cnt;
	msg->btf_cnt = stats.btf_cnt;
	msg->tx_wrong_size_cnt = stats.tx_wrong_size_cnt;
	msg->rx_wrong_size_cnt = stats.rx_wrong_size_cnt;
}


static void _process_input_packets()
{
	mavlink_message_t input_msg;
	int rc = mav_main_get_packet_from_its_link(&input_msg);
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


