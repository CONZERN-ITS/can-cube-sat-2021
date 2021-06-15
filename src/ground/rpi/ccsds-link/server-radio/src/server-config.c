#include "server-config.h"

#include <errno.h>
#include <stdio.h>

#include <log.h>


int server_config_init(server_config_t * config)
{
	memset(config, 0x00, sizeof(*config));
	return 0;
}


int server_config_load(server_config_t * config)
{
	const sx126x_drv_basic_cfg_t basic_cfg = {
			.use_dio3_for_tcxo = true,
			.tcxo_v = SX126X_TCXO_CTRL_1_8V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.standby_mode = SX126X_STANDBY_XOSC
	};
	config->radio_basic_cfg = basic_cfg;

	const sx126x_drv_lora_modem_cfg_t modem_cfg = {
			// Параметры усилителей и частот
			.frequency = 438125*1000,
			.pa_ramp_time = SX126X_PA_RAMP_3400_US,
			.pa_power = 10,
			.lna_boost = true,

			// Параметры пакетирования
			.spreading_factor = SX126X_LORA_SF_8,
			.bandwidth = SX126X_LORA_BW_250,
			.coding_rate = SX126X_LORA_CR_4_8,
			.ldr_optimizations = false,
	};
	config->radio_modem_cfg = modem_cfg;

	const sx126x_drv_lora_packet_cfg_t packet_cfg = {
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,
			.preamble_length = 8,
			.explicit_header = true,
			.payload_length = 200,
			.use_crc = true,
	};
	config->radio_packet_cfg = packet_cfg;

	const sx126x_drv_cad_cfg_t cad_cfg = {
			.cad_len = SX126X_LORA_CAD_04_SYMBOL,
			.cad_min = 10,
			.cad_peak = 28,
			.exit_mode = SX126X_LORA_CAD_RX,
	};
	config->radio_cad_cfg = cad_cfg;

	const sx126x_drv_lora_rx_timeout_cfg_t rx_timeout_cfg = {
			.stop_timer_on_preamble = false,
			.lora_symb_timeout = 0,
	};
	config->radio_rx_timeout_cfg = rx_timeout_cfg;


	config->rx_timeout_ms = 600;
	config->rx_timeout_limit = 0;

	config->tx_timeout_ms = 0;

	config->rx_watchdog_ms = 5000;
	config->tx_watchdog_ms = 5000;

	config->tx_state_report_period_ms = 500;
	config->rssi_report_period_ms = 50;
	config->radio_stats_report_period_ms = 2000;
	config->poll_timeout_ms = 50;

	return 0;
}


void server_config_destroy(server_config_t * config)
{

}

