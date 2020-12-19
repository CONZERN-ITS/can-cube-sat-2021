#ifndef RADIO_SX126X_DRV_H_
#define RADIO_SX126X_DRV_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sx126x_platform.h"
#include "sx126x_defs.h"


typedef enum sx126x_drv_state_t
{
	SX126X_DRVSTATE_ERROR,
	SX126X_DRVSTATE_STANDBY_RC,
	SX126X_DRVSTATE_STANDBY_XOSC,
	SX126X_DRVSTATE_LBT_IDLE,
	SX126X_DRVSTATE_LBT_RX,
	SX126X_DRVSTATE_LBT_TX,
} sx126x_drv_state_t;


typedef enum sx126x_drv_txcplt_flags_t
{
	SX126X_DRV_TXCPLT_FLAGS_FAILED
} sx126x_drv_txcplt_flags_t;


typedef enum sx126x_drv_rx_callback_flags_t
{
	SX126X_DRV_RXCB_FLAGS_BAD_CRC
} sx126x_drv_rx_callback_flags_t;


typedef struct sx126x_drv_chip_cfg_t
{
	// Настройки IO и питания
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! Разрешает использование пина DIO3 для управления TCXO
	/*! Эта опция поддерживает только включение. Отключение только через полный резет чипа */
	bool use_dio3_for_tcxo;
	//! Напряжение питания TCXO
	sx126x_tcxo_v_t tcxo_v;
	//! Разрешает использование пина DIO2 для включения PA антенны
	bool use_dio2_for_rf_switch;
	//! Разрешить использование DCDC (должен быть разведен на модуле)
	bool allow_dcdc;

	// Настройки клоков
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! Управляет отключением осциляторов при выходе из TX/RX режима
	sx126x_fallback_mode_t rxtx_fallback_mode;

} sx126x_drv_chip_cfg_t;


//! Набор параметров лора модема
typedef struct sx126x_drv_lora_cfg_t
{
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//! Частота, на которой будет работать трансивер
	uint32_t frequency;

	// Настройки PA усилителя
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

	// Параметры модуляции
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	sx126x_lora_sf_t spreading_factor;
	sx126x_lora_bw_t bandwidth;
	sx126x_lora_cr_t coding_rate;
	bool ldr_optimizations;

	// Параметры пакетизации
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	uint16_t preamble_length;
	bool explicit_header;
	uint8_t payload_length;
	bool use_crc;
	bool invert_iq;
	sx126x_lora_syncword_t syncword;

	// Всякие прочие параметры
	bool boost_rx_lna;		//!< Использовать усиление встроенного LNA на приёме

	// Carrier detect параметры
	//! Минимальное время в течение которого модуль будет пытаться получить пакет
	//! Перед тем как канал будет считаться свободным
	uint32_t rx_min_time;
	sx126x_cad_length_t cad_len;
	uint8_t cad_peak;
	uint8_t cad_min;
} sx126x_drv_lora_cfg_t;


//! Колбек, который вызывается драйвером, когда он готов принять пакет на отправку
/*! Функция должна записать пакет в буфер по адресу dest_buffer, не превышая размера dest_buffer_size
	Функция должна вернуть количество записанных в буфер байт.
	Поскольку радиомодуль умеет отправлять пакеты только фиксированной длины -
	будет отправлен весь буфер как он есть, даже если пользователь записал только лишь его часть
	Если сейчас нет пригодного для передачи пакета - функция должна вернуть значение  0. */
typedef uint8_t (*sx126x_drv_ontx_callback_t)(
		void * /*user_arg*/,
		uint8_t * /*dest_buffer*/,
		uint8_t /*dest_buffer_size*/,
		int * /*flags*/,
		int * /*cookie*/
);

//! Колбек, который вызывается драйвером, когда пакет был отправлен (или нет)
typedef void (*sx126x_drv_txcplt_callback_t)(
		void * /* user_arg */,
		int /* cookie */,
		int /* flags */
);

//! Колбек, который вызывается драйвером, когда он готов передать пакет приложению
typedef void (*sx126x_drv_onrx_callback_t)(
		void * /*user_arg*/,
		const uint8_t * /*packet_data*/,
		uint8_t /*packet_data_size*/,
		int /*flags*/
);


typedef struct sx126x_drv_callback_cfg_t
{
	//! Пользовательский аргумент для передачи в колбеки по событиям
	void * cb_user_arg;
	//! Колбек, который вызывается драйвером, когда он готов принять пакет на отправку
	sx126x_drv_ontx_callback_t ontx_callback;

	//! Колбек, который вызывается драйвером после того пакет был отправлен
	sx126x_drv_txcplt_callback_t ontxcplt_callback;

	//! колбек, который вызывается драйвером, когда он готов передать пакет приложению
	sx126x_drv_onrx_callback_t onrx_callback;
} sx126x_drv_callback_cfg_t;


//! Дескриптор устройства
typedef struct sx126x_dev_t
{
	sx126x_drv_state_t state;
	sx126x_plt_t plt;

	uint8_t payload_size;
	sx126x_packet_type_t packet_type;
	bool explicit_lora_header;

	uint32_t rx_timeout_hard;
	uint32_t rx_timeout_soft;
	uint32_t tx_timeout_hard;
	uint32_t tx_timeout_soft;
	uint32_t soft_timeout_start;

	uint8_t rx_buffer[0xFF];
	uint8_t rx_packet_size;
	int rx_packet_flags;

	uint8_t tx_buffer[0xFF];
	uint8_t tx_packet_size;
	int tx_buffer_flags;
	int tx_packet_cookie;
	int tx_packet_pending_cookie;

	int tx_cplt_flags;
	int tx_cplt_cookie;
	bool tx_cplt_pending;

	void * cb_user_arg;
	sx126x_drv_ontx_callback_t ontx_callback;
	sx126x_drv_txcplt_callback_t ontxcplt_callback;
	sx126x_drv_onrx_callback_t onrx_callback;
} sx126x_dev_t;


//! Конструктор драйвера
int sx126x_drv_ctor(sx126x_dev_t * dev, void * plt_init_user_arg);

//! Деструктор драйвера
void sx126x_drv_dtor(sx126x_dev_t * dev);

//! Сброс чипа и состояния драйвера через RST пин
int sx126x_drv_reset(sx126x_dev_t * dev);

//! Перевод чипа в состояние standby_rc
/*! Это самый базовый standby режим в котором включен самый минимум железа в чипе,
	который позволяет ему воспринимать команды. В частности чип едет на медленном RC осциляторе.
	Для работы по радио нужен XOSC осциялятор, который в этом режиме не включен.
	В этом режиме чип полагается конфигурировать */
int sx126x_drv_mode_standby_rc(sx126x_dev_t * dev);

//! Базовые настройки чипа безотносительно модемов
/*! Поскольку базовые настройки влияют на самый базовые блоки чипа - эти настойки необходимо
	применять тогда, когда чип в standby_rc режиме */
int sx126x_drv_configure_chip(sx126x_dev_t * dev, const sx126x_drv_chip_cfg_t * config);

//! Перевод чипа в standby_xosc.
/*! В этом режиме запущены дополнительные осциялторы. Из этого режима можно быстро прыгнуть в RX или TX режимы */
int sx126x_drv_mode_standby_xosc(sx126x_dev_t * dev);

//! Конфигурация лора модема
int sx126x_drv_configure_lora(sx126x_dev_t * dev, const sx126x_drv_lora_cfg_t * config);

int sx126x_drv_configure_callbacks(sx126x_dev_t * dev, const sx126x_drv_callback_cfg_t * config);

int sx126x_drv_start(sx126x_dev_t * dev);

int sx126x_drv_poll(sx126x_dev_t * dev);


#endif /* RADIO_SX126X_DRV_H_ */
