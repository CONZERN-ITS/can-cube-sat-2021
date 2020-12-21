#ifndef RADIO_SX126X_DRV_H_
#define RADIO_SX126X_DRV_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sx126x_defs.h"
#include "sx126x_api.h"
#include "sx126x_events.h"


typedef enum sx126x_drv_state_t
{
	SX126X_DRVSTATE_ERROR,
	SX126X_DRVSTATE_STANDBY_RC,
	SX126X_DRVSTATE_STANDBY_DEFAULT,
	SX126X_DRVSTATE_TX,
	SX126X_DRVSTATE_RX,
	SX126X_DRVSTATE_CAD,
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

	// Настройки усилителей усилителя
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! "Скорость разгона" PA усилителя?
	sx126x_pa_ramp_time_t pa_ramp_time;

	//! Мощность PA усилителя
	/*! Фактическая мощность подбирается из предопределенных в даташите значений
		для разных чипов. Указанное здесь значение приводится к ближайшему определенному.
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


//! Параметры модема, используемые в LoRa TX режиме
typedef struct sx126x_drv_lora_tx_cfg_t
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

	//! Таймаут на передачу пакета
	/*! Если указан 0 - таймаут отключен */
	uint32_t tx_timeout_ms;

} sx126x_drv_lora_tx_cfg_t;


//! Параметры модема, используемые в LoRa RX режиме
typedef struct sx126x_drv_lora_rx_cfg_t
{
	//! Ожидать инверсию IQ
	bool invert_iq;
	//! Ожидать синхрослово для публичной LoRa сети или для приватной
	sx126x_lora_syncword_t syncword;
	//! Длина ожидаемой преамбулы
	/*! (Если длина ожидаемой преамбулы неизвестна - следует установить значение 0xFFFF) */
	uint16_t preamble_length;
	//! Длина ожидаемого пакета
	/*! Если установить это поле в 0, то будет включен режим явного заголовка */
	bool payload_length;
	//! Добавлять ли к пакету контрольную сумму
	/*! Если payload_length == 0, поле не имеет значения */
	bool use_crc;

	//! Количество миллисекунд таймаута для rx операции
	uint32_t rx_timeout_ms;
	//! Альтернативный режим таймаута в количествах символов лоры
	uint8_t rx_timeout_symbs;
	//! Разрешить остановку RX таймера по преамбуле
	bool rx_timeout_stop_on_preamble;

} sx126x_drv_lora_rx_cfg_t;


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
	sx126x_drv_state_t state;

	sx126x_standby_mode_t _default_standby;

	sx126x_evt_handler_t _evt_handler;
	void * _evt_handler_user_arg;

	union
	{
		struct
		{
			sx126x_drv_lora_tx_cfg_t tx_cfg;
			sx126x_drv_lora_rx_cfg_t rx_cfg;
			sx126x_lora_bw_t bw;
			sx126x_cad_exit_mode_t cad_exit_mode;
		} lora;

		struct
		{
			int not_implemented;
		} gfsk;

	} _modem_state;
	sx126x_packet_type_t _modem_type;

} sx126x_drv_t;


//! Конструктор драйвера
int sx126x_drv_ctor(sx126x_drv_t * drv, void * board_ctor_arg);

//! Деструктор драйвера
void sx126x_drv_dtor(sx126x_drv_t * drv);

//! Регистрация обработчика событий драйвера
int sx126x_drv_register_event_handler(sx126x_drv_t * drv, sx126x_evt_handler_t handler, void * cb_user_arg);

//! Сброс чипа через RST пин и состояния драйвера
int sx126x_drv_reset(sx126x_drv_t * drv);

//! Переводит чип в режим standby на RC цепочке
/*! Это самый базовый режим в котором отключено все что можно.
	Этот режим следует использовать для проведения базовой конфигурации чипа */
int sx126x_drv_mode_standby_rc(sx126x_drv_t * drv);

//! Переводит чип в тот вид standby режима, который указан в конфиге драйвера
int sx126x_drv_mode_standby(sx126x_drv_t * drv);

int sx126x_drv_mode_rx(sx126x_drv_t * drv);

int sx126x_drv_mode_tx(sx126x_drv_t * drv);

int sx126x_drv_mode_cad(sx126x_drv_t * drv);

int sx126x_drv_payload_write(sx126x_drv_t * drv, const uint8_t * data, uint8_t data_size);

int sx126x_drv_payload_rx_size(sx126x_drv_t * drv, uint8_t * data_size);

int sx126x_drv_payload_read(sx126x_drv_t * drv, uint8_t * buffer, uint8_t buffer_size);

//! Базовая конфигурация чипа и драйвера
/*! Большинство опций, которые используются здесь не поддерживают
	повторной реконфигурации. Поэтому эта функция должна использоваться
	один раз после резета чипа в режиме standby_rc */
int sx126x_drv_configure_basic(sx126x_drv_t * drv, const sx126x_drv_basic_cfg_t * config);

//! Активация и конфигурирование LoRa модема
int sx126x_drv_configure_lora_modem(sx126x_drv_t * drv, const sx126x_drv_lora_modem_cfg_t * config);

//! Настройка CAD параметров LoRa модема
/*! В отличии от RX/TX конфигурации - эта конфигурация загружается в модуль прямо в этом вызове */
int sx126x_drv_configure_lora_cad(sx126x_drv_t * drv, const sx126x_drv_cad_cfg_t * config);

//! Конфигурация лоры, используемая при отправке пакетов
/*! Конфигурация не применяется прямо сейчас. Она будет загружена в модем перед перходом в TX */
int sx126x_drv_set_lora_tx_config(sx126x_drv_t * drv, const sx126x_drv_lora_tx_cfg_t * config);

//! Конфигурация лоры, используемая при приёме пакетов
/*! Конфигурация не применяется прямо сейчас. Она будет загружена в модем перед перходом в RX */
int sx126x_drv_set_lora_rx_config(sx126x_drv_t * drv, const sx126x_drv_lora_rx_cfg_t * config);

//! Обработка событий драйвера
int sx126x_drv_poll(sx126x_drv_t * drv);


#endif /* RADIO_SX126X_DRV_H_ */
