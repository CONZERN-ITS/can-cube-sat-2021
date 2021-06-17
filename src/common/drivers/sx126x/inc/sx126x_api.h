#ifndef RADIO_SX126X_API_H_
#define RADIO_SX126X_API_H_

#include <stdint.h>
#include <stdbool.h>

#include "sx126x_defs.h"
#include "sx126x_board.h"


#define SX126X_API_CALIBRATE_IMAGE_ARG_LEN (2)
#define SX126X_API_LORA_PACKET_PARAMS_ARG_LEN (4)
#define SX126X_API_GFSK_PACKET_PARAMS_ARG_LEN (9)
#define SX126X_API_PACKET_STATUS_ARG_LEN (3)

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


typedef struct sx126x_api_t
{
	sx126x_board_t * board;
} sx126x_api_t;


int sx126x_api_ctor(sx126x_api_t * api, void * board_ctor_arg);
void sx126x_api_dtor(sx126x_api_t * api);

int sx126x_api_set_sleep(sx126x_api_t * api, uint8_t sleep_flags);
int sx126x_api_set_standby(sx126x_api_t * api, sx126x_standby_mode_t mode);
int sx126x_api_set_fs(sx126x_api_t * api);
int sx126x_api_set_tx(sx126x_api_t * api, uint32_t timeout_ms);
int sx126x_api_set_rx(sx126x_api_t * api, uint32_t timeout_ms);
int sx126x_api_stop_rx_timer_on_preamble(sx126x_api_t * api, bool enable);
int sx126x_api_set_rx_duty_cycle(sx126x_api_t * api, uint32_t rx_time_ms, uint32_t sleep_time_ms);
int sx126x_api_set_cad(sx126x_api_t * api);
int sx126x_api_set_tx_continous_wave(sx126x_api_t * api);
int sx126x_api_set_tx_continous_preamble(sx126x_api_t * api);
int sx126x_api_set_regulator_mode(sx126x_api_t * api, sx126x_regulator_t mode);
int sx126x_api_calibrate(sx126x_api_t * api, uint8_t calib_flags);
int sx126x_api_calibrate_image(sx126x_api_t * api, const uint8_t * args);
int sx126x_api_set_pa_config(sx126x_api_t * api,
		uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut
);
int sx126x_api_set_rxtx_fallback_mode(sx126x_api_t * api, sx126x_fallback_mode_t mode);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int sx126x_api_set_dio_irq_params(sx126x_api_t * api,
		uint16_t global_irq_mask, uint16_t dio1_irq_mask, uint16_t dio2_irq_mask, uint16_t dio3_irq_mask
);
int sx126x_api_get_irq_status(sx126x_api_t * api, uint16_t * irq_status);
int sx126x_api_clear_irq_status(sx126x_api_t * api, uint16_t clear_mask);
int sx126x_api_set_dio2_as_rf_switch(sx126x_api_t * api, bool enable);
int sx126x_api_set_dio3_as_tcxo_ctl(sx126x_api_t * api, sx126x_tcxo_v_t tcxo_voltage, uint32_t timeout_ms);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int sx126x_api_set_rf_frequency(sx126x_api_t * api, uint32_t frequency);
int sx126x_api_set_packet_type(sx126x_api_t * api, sx126x_packet_type_t packet_type);
int sx126x_api_get_packet_type(sx126x_api_t * api, sx126x_packet_type_t * packet_type);
int sx126x_api_set_tx_params(sx126x_api_t * api, int8_t power, sx126x_pa_ramp_time_t pa_ramp_time);

int sx126x_api_set_modulation_params(sx126x_api_t * api, const uint8_t * raw_args, uint16_t raw_args_size);
int sx126x_api_set_gfsk_modulation_params(sx126x_api_t * api, const sx126x_gfsk_modulation_params_t * params);
int sx126x_api_set_lora_modulation_params(sx126x_api_t * api, const sx126x_lora_modulation_params_t * params);

int sx126x_api_set_packet_params(sx126x_api_t * api, const uint8_t * raw_args, uint16_t raw_args_size);
int sx126x_api_set_gfsk_packet_params(sx126x_api_t * api, const sx126x_gfsk_packet_params_t * params);
int sx126x_api_set_lora_packet_params(sx126x_api_t * api, const sx126x_lora_packet_params_t * params);

int sx126x_api_set_cad_params(sx126x_api_t * api,
		sx126x_cad_length_t cad_len, uint8_t cad_peak, uint8_t cad_min,
		sx126x_cad_exit_mode_t exit_mode, uint32_t cad_timeout_ms
);

int sx126x_api_set_buffer_base_address(sx126x_api_t * api, uint8_t tx_base_addr, uint8_t rx_base_addr);
int sx126x_api_set_lora_symb_num_timeout(sx126x_api_t * api, uint8_t symb_num);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int sx126x_api_get_status(sx126x_api_t * api, sx126x_status_t * status);
int sx126x_api_get_rx_buffer_status(sx126x_api_t * api, uint8_t * payload_size, uint8_t * payload_offset);

int sx126x_api_get_packet_status(sx126x_api_t * api, uint8_t * raw_data_buffer);
int sx126x_api_get_gfsk_packet_status(sx126x_api_t * api, sx126x_gfsk_packet_status_t * status);
int sx126x_api_get_lora_packet_status(sx126x_api_t * api, sx126x_lora_packet_status_t * status);

int sx126x_api_get_rssi_inst(sx126x_api_t * api, int8_t * rssi);

int sx126x_api_get_stats(sx126x_api_t * api, sx126x_stats_t * stats);
int sx126x_api_reset_stats(sx126x_api_t * api);

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int sx126x_api_get_device_errors(sx126x_api_t * api, uint16_t * device_errors);
int sx126x_api_clear_device_errors(sx126x_api_t * api);

#endif /* RADIO_SX126X_API_H_ */
