#include "server-config.h"


#include <sx126x_defs.h>


int server_config_load(server_config_t * config)
{
	config->tx_buffer_capacity =
	config->radio_packet_size = 200;

	config->rx_timeout_ms = 1000;
	config->rx_timeout_limit = 5;
	config->tx_timeout_ms = 50000;

	config->tx_state_report_period_ms = 500;
	config->rssi_report_period_ms = 500;
	config->radio_stats_report_period_ms = 1000;
	config->poll_timeout_ms = 1000;

	config->ignored_errors = (SX126X_DEVICE_ERROR_XOSC_START|0);

	return 0;
}
