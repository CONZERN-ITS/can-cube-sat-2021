#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sx126x_drv.h>


typedef struct server_config_t
{
	//! Аргумент sx126x_drv_mode_rx */
	/*! если используется sx126x_drv_lora_rx_timeout_cfg_t::lora_symb_timeout то таймаут считается в символах
		а не в милллисекундах */
	uint32_t rx_timeout_ms;
	//! Сколько раз должен случится таймаут на RX, чтобы было разрешено делать TX
	size_t rx_timeout_limit;
	//! Аргумент sx126x_drv_mode_tx */
	uint32_t tx_timeout_ms;

	//! Программный таймаут на RX
	/*! Если в течение этого времени от радио не поступит никаких сигналов
		Оно будет перезапущено */
	uint32_t rx_watchdog_ms;
	//! Программный таймаут на TX
	/*! Если в течение этого времени от радио не поступит никаких сигналов
		Оно будет перезапущено */
	uint32_t tx_watchdog_ms;

	//! Насколько часто сервер будет публиковать сообщение о очереди TX пакетов
	uint32_t tx_state_report_period_ms;
	//! Насколько часто в сервер будет публировать RSSI сообщение в режиме приёма
	uint32_t rssi_report_period_ms;
	//! Насколько часто сервер будет публиковать состояние радио
	uint32_t radio_stats_report_period_ms;
	//! Период проверки завешения RX или TX режима на радио
	uint32_t poll_timeout_ms;

	//! Использовать ли первый байт пейлоада как номер фрейма
	bool extract_frame_number;

	// Дальше идут настройки радио-драйвера
	// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

	//! Базовые настройки радио
	sx126x_drv_basic_cfg_t radio_basic_cfg;
	//! Настройки лора-модема радио
	sx126x_drv_lora_modem_cfg_t radio_modem_cfg;
	//! Настройки лора-пакетизатора радио
	sx126x_drv_lora_packet_cfg_t radio_packet_cfg;
	//! Настройки CAD режима радио
	sx126x_drv_cad_cfg_t radio_cad_cfg;
	//! Настройки поведения RX таймаута на радио
	sx126x_drv_lora_rx_timeout_cfg_t radio_rx_timeout_cfg;

} server_config_t;


//! Иницализация структуры конфига сервера
int server_config_init(server_config_t * config);
//! Загрузка конфигурации конфига сервера из указанного файла
int server_config_load(server_config_t * config);
//! Удаление структуры конфига сервера
void server_config_destroy(server_config_t * config);

#endif /* SRC_CONFIG_H_ */
