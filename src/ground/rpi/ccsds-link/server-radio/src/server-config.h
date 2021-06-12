#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef struct server_config_t
{
	size_t tx_buffer_capacity;
	size_t radio_packet_size;

	uint32_t rx_timeout_ms;
	size_t rx_timeout_limit;
	uint32_t tx_timeout_ms;

	uint32_t tx_state_report_period_ms;
	uint32_t rssi_report_period_ms;
	uint32_t radio_stats_report_period_ms;
	uint32_t poll_timeout_ms;

	uint16_t ignored_errors;
} server_config_t;


int server_config_load(server_config_t * config);


#endif /* SRC_CONFIG_H_ */
