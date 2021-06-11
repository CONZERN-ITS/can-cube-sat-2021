#ifndef RADIO_SX126X_DRV_H_
#define RADIO_SX126X_DRV_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sx126x_defs.h"
#include "sx126x_api.h"


//! Тип события драйвера
typedef enum sx126x_drv_evt_kind_t
{
	//! Никакого события не произошло
	SX126X_EVTKIND_NONE = 0x00,
	//! Передача закончена
	SX126X_EVTKIND_TX_DONE,
	//! Приём закончен
	SX126X_EVTKIND_RX_DONE
} sx126x_drv_evt_kind_t;


//! параметры события драйвера
typedef union sx126x_drv_evt_arg_t
{
	//! параметры события rx_done
	struct
	{
		//! Случился ли аппаратный таймаут
		bool timed_out;
		//! Верна ли контрольная сумма пакета
		bool crc_valid;
	} rx_done;

	//! Параметры события tx_done
	struct
	{
		//! случился ли аппаратный таймаут
		bool timed_out;
	} tx_done;

} sx126x_drv_evt_arg_t;


//! Событие драйвера
typedef struct sx126x_drv_evt_t
{
	//! тип случившегося события
	sx126x_drv_evt_kind_t kind;
	//! Его аргументы
	sx126x_drv_evt_arg_t arg;
} sx126x_drv_evt_t;


//! Тип модема на который настроен модуль
typedef enum sx126x_drv_modem_type_t
{
	//! Модем вообще не настроен
	SX126X_DRV_MODEM_TYPE_UNCONFIGURED,
	//! Используется LoRa модем
	SX126X_DRV_MODEM_TYPE_LORA,
	// Используется GFSK модем
	//SX126X_DRV_MODEM_STATE_GFSK, // TODO: Not implemented
} sx126x_drv_modem_type_t;


//! Состояние драйвера
typedef enum sx126x_drv_state_t
{
	// TODO: CAD, SLEEP и RX_DUTY_CYCLE режимы

	//! Устройство только что включилось и толком не отконфигурированно
	SX126X_DRV_STATE_STARTUP = 1,
	//! Устройство в standby на RC цепочке
	SX126X_DRV_STATE_STANDBY_RC,
	//! Устройство в STANDBY на внешнем генераторе
	SX126X_DRV_STATE_STANDBY_XOSC,
	//! Устройство передает!
	SX126X_DRV_STATE_TX,
	//! Устройство принимает
	SX126X_DRV_STATE_RX,
} sx126x_drv_state_t;


//! Базовая конфигурация чипа и драйвера
typedef struct sx126x_drv_basic_cfg_t
{
	//! Разрешает использование пина DIO3 для управления TCXO
	bool use_dio3_for_tcxo;
	//! Напряжение питания TCXO
	sx126x_tcxo_v_t tcxo_v;
	//! Разрешает использование пина DIO2 для включения PA антенны
	bool use_dio2_for_rf_switch;
	//! Разрешить использование DCDC
	bool allow_dcdc;
	//! режим standby состояния драйвера
	sx126x_standby_mode_t standby_mode;
} sx126x_drv_basic_cfg_t;


//! Настройки LoRa модема
typedef struct sx126x_drv_lora_modem_cfg_t
{
	// =-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! Частота, на которой будет работать трансивер
	uint32_t frequency;

	// Настройки усилителя
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! "Скорость разгона" PA усилителя?
	sx126x_pa_ramp_time_t pa_ramp_time;

	//! Мощность PA усилителя
	/*! Фактическая мощность подбирается из предопределенных в даташите значений
		для разных чипов. Указанное здесь значение приводится к ближайшему определенному
		не выше указанного
		для sx1261 определены значения 10, 14, 15 dBm
		для sx1262 определены значения 14, 17, 20, 22 dBm
		для sx1268 определены значения 10, 14, 17, 20, 22 dBm */
	int8_t pa_power;

	//! Использовать ли буст LNA усилителя
	bool lna_boost;

	// Параметры модуляции
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! LoRa spreading factor
	sx126x_lora_sf_t spreading_factor;
	//! Полоса вещания
	sx126x_lora_bw_t bandwidth;
	//! Избыточность помехозащищенного кодирования
	sx126x_lora_cr_t coding_rate;
	//! использовать ли low data rate оптимизации
	bool ldr_optimizations;
} sx126x_drv_lora_modem_cfg_t;


//! Параметры пакетизатора LoRa
typedef struct sx126x_drv_lora_packet_cfg_t
{
	//! Использовать инверсию IQ
	bool invert_iq;
	//! Использовать синхрослово для публичной LoRa сети или для приватной
	sx126x_lora_syncword_t syncword;
	//! Длина отправляемой преамбулы
	uint16_t preamble_length;
	//! Использовать явный LoRa заголовок
	bool explicit_header;
	//! Длина передаваемого пакета
	uint8_t payload_length;
	//! Использовать ли CRC в теле пакета
	bool use_crc;

} sx126x_drv_lora_packet_cfg_t;


//! Настройки RX таймаута лоры
typedef struct sx126x_drv_lora_rx_timeout_cfg_t
{
	//! Дополнительный таймаут на количество обнаруженных LoRa символов
	uint8_t lora_symb_timeout;
	//! Разрешает остановку rx таймаут таймера по получению символов преамбулы
	bool stop_timer_on_preamble;
} sx126x_drv_lora_rx_timeout_cfg_t;


//! Параметры модема, используемые в LoRa CAD режиме
typedef struct sx126x_drv_cad_cfg_t
{
	sx126x_cad_length_t cad_len;
	uint8_t cad_peak;
	uint8_t cad_min;
	sx126x_cad_exit_mode_t exit_mode;
	uint32_t cad_timeout_ms;
} sx126x_drv_cad_cfg_t;


//! Дескриптор устройства
typedef struct sx126x_drv_t
{
	sx126x_api_t api;
	sx126x_drv_state_t _state;

	sx126x_standby_mode_t _default_standby;
	bool _infinite_rx;

	union
	{
		struct
		{
			bool explicit_header;
			uint8_t payload_length;
			sx126x_lora_bw_t bw;
			sx126x_cad_exit_mode_t cad_exit_mode;
		} lora;

		struct
		{
			int not_implemented;
		} gfsk;

	} _modem_state;
	sx126x_drv_modem_type_t _modem_type;

} sx126x_drv_t;


// Функции для первичной инициализации драйвера
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Конструктор драйвера
int sx126x_drv_ctor(sx126x_drv_t * drv, void * board_ctor_arg);

//! Деструктор драйвера
void sx126x_drv_dtor(sx126x_drv_t * drv);

//! Состояние драйвера
sx126x_drv_state_t sx126x_drv_state(sx126x_drv_t * drv);


// Функции для первичной инициализации железки, сбросов и standby режимов
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Сброс чипа через RST пин и состояния драйвера
int sx126x_drv_reset(sx126x_drv_t * drv);

//! Базовая конфигурация чипа и драйвера
/*! Большинство опций, которые используются здесь не поддерживают
	повторной реконфигурации. Поэтому эта функция должна использоваться
	один раз после резета чипа.
	Функция может быть вызвана только в состоянии SX126X_DRV_STATE_STARTUP
	И только посредством этой функции можно перейти в первый стендбай rc */
int sx126x_drv_configure_basic(sx126x_drv_t * drv, const sx126x_drv_basic_cfg_t * config);

//! Переводит чип в режим standby на RC цепочке
/*! Это самый базовый режим в котором отключено все что можно.
	Этот режим следует использовать для проведения базовой конфигурации чипа */
int sx126x_drv_mode_standby_rc(sx126x_drv_t * drv);

//! Переводит чип в тот вид standby режима, который указан в конфиге драйвера
int sx126x_drv_mode_standby(sx126x_drv_t * drv);


// Функции для вторичной инициализации железки. Настройки модема и пакетирования
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Эти функции должны быть вызваны после configure_basic и очевидно из standby режимов

//! Активация и конфигурирование LoRa модема
int sx126x_drv_configure_lora_modem(sx126x_drv_t * drv, const sx126x_drv_lora_modem_cfg_t * config);

//! Конфигурация пакетизатора LoRa
/*! Должна быть вызвана после sx126x_drv_configure_lora_modem */
int sx126x_drv_configure_lora_packet(sx126x_drv_t * drv, const sx126x_drv_lora_packet_cfg_t * config);

//! Настройка CAD параметров LoRa модема
int sx126x_drv_configure_lora_cad(sx126x_drv_t * drv, const sx126x_drv_cad_cfg_t * config);

//! Настройки rx таймаут таймера LoRa модема
int sx126x_drv_configure_lora_rx_timeout(sx126x_drv_t * drv, const sx126x_drv_lora_rx_timeout_cfg_t * config);

//! Параметры последнего полученного пакета
int sx126x_drv_lora_packet_status(sx126x_drv_t * drv, sx126x_lora_packet_status_t * status);

// Функции для работы с буффером данных
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Этот драйвер использует буфер целиком и для RX и для TX операций

//! Запись данных в отправной буфер
int sx126x_drv_payload_write(sx126x_drv_t * drv, const uint8_t * data, uint8_t data_size);

//! Получение размера последнего полученного пакета
int sx126x_drv_payload_rx_size(sx126x_drv_t * drv, uint8_t * data_size);

//! Чтение содержимого буфера
int sx126x_drv_payload_read(sx126x_drv_t * drv, uint8_t * buffer, uint8_t buffer_size);

// Функции для режимов передачи данных и приёма
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Переход в режим приёма
/*! Если таймаут указан 0, то таймаута нет и модуль останется в RX до получения первого пакета.
    После получения первого пакета модуль вернется в standby
    Если таймаут указан 0xFFFFFF, то таймаута нет и модуль останется в RX даже после получения
    первого пакета и в standby не вернется пока не будет указано явно */
int sx126x_drv_mode_rx(sx126x_drv_t * drv, uint32_t timeout_ms);

//! Переход в режим передачи
/*! Если таймаут указан 0, то таймаута нет и модуль останется в TX до полной передачи пакета */
int sx126x_drv_mode_tx(sx126x_drv_t * drv, uint32_t timeout_ms);

// Функции для работы с событиями драйвера
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Опрос модуля на предмет новых событий в радио
int sx126x_drv_poll_event(sx126x_drv_t * drv, sx126x_drv_evt_t * evt);

//! Блокирующее ожидание новых событий в радио
int sx126x_drv_wait_event(sx126x_drv_t * drv, uint32_t timeout_ms, sx126x_drv_evt_t * evt);

// Функции для блокирующего интерфейса
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int sx126x_drv_tx_blocking(sx126x_drv_t * drv,
		uint32_t hw_timeout_ms, uint32_t sw_timeout_ms,
		sx126x_drv_evt_t * termination_event
);

int sx126x_drv_rx_blocking(sx126x_drv_t * drv,
		uint32_t hw_timeout_ms, uint32_t sw_timeout_ms,
		sx126x_drv_evt_t * termination_event
);

// Функции для различных опросов модуля
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//! Получение текущего значения RSSI, определяеого модулем.
/*! Очевидно имеет смысл вызывать только в RX/CAD режимах работы модуля */
int sx126x_drv_rssi_inst(sx126x_drv_t * drv, int8_t * value);

//! Получение статистики, накопленной модулем
int sx126x_drv_get_stats(sx126x_drv_t * drv, sx126x_stats_t * stats);

//! Получение ошибок радио
int sx126x_drv_get_device_errors(sx126x_drv_t * drv, uint16_t * error_bits);

//! Сброс флагов ошибок устройства
int sx126x_drv_clear_device_errors(sx126x_drv_t * drv);

#endif /* RADIO_SX126X_DRV_H_ */
