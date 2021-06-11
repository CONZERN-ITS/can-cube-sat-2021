#include "sx126x_api.h"


// translation between milliseconds and sx126x timeout units
#define SX126X_MS_TO_TIME_U(ms)	((ms) << 6)
#define SX126X_RETURN_IF_NONZERO(rc) if (0 != rc) return rc

#define SX126X_F_XTAL		32000000
#define SX126X_2_POW_25		33554432 // pow( 2.0, 25.0 )


int sx126x_api_ctor(sx126x_api_t * api, void * board_ctor_arg)
{
	int rc;

	rc = sx126x_brd_ctor(&api->board, board_ctor_arg);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


void sx126x_api_dtor(sx126x_api_t * api)
{
	sx126x_brd_dtor(api->board);
}


int sx126x_api_set_sleep(sx126x_api_t * api, uint8_t sleep_flags)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_SLEEP, &sleep_flags, sizeof(sleep_flags));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_standby(sx126x_api_t * api, sx126x_standby_mode_t mode)
{
	int rc;
	const uint8_t mode_value = mode;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_STANDBY, &mode_value, sizeof(mode_value));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_fs(sx126x_api_t * api)
{
	int rc;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_FS, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_tx(sx126x_api_t * api, uint32_t timeout_ms)
{
	int rc;

	const uint32_t timeout = SX126X_MS_TO_TIME_U(timeout_ms);
	uint8_t timeout_bytes[3] = {
			(uint8_t)((timeout >> 16) & 0xFF),
			(uint8_t)((timeout >>  8) & 0xFF),
			(uint8_t)((timeout      ) & 0xFF)
	};

	rc = 0;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_TX, timeout_bytes, sizeof(timeout_bytes));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_rx(sx126x_api_t * api, uint32_t timeout_ms)
{
	int rc;
	const uint32_t timeout = SX126X_MS_TO_TIME_U(timeout_ms);

	uint8_t timeout_bytes[3] = {
			(uint8_t)((timeout >> 16) & 0xFF),
			(uint8_t)((timeout >>  8) & 0xFF),
			(uint8_t)((timeout      ) & 0xFF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_RX, timeout_bytes, sizeof(timeout_bytes));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_stop_rx_timer_on_preamble(sx126x_api_t * api, bool enable)
{
	int rc;

	const uint8_t arg = enable ? 1 : 0;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_STOPRXTIMERONPREAMBLE, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_rx_duty_cycle(sx126x_api_t * api, uint32_t rx_time_ms, uint32_t sleep_time_ms)
{
	int rc;
	const uint32_t rx_time = SX126X_MS_TO_TIME_U(rx_time_ms);
	const uint32_t sleep_time = SX126X_MS_TO_TIME_U(sleep_time_ms);

	uint8_t args[6] = {
			(uint8_t)((rx_time >> 16) & 0xFF),
			(uint8_t)((rx_time >>  8) & 0xFF),
			(uint8_t)((rx_time      ) & 0xFF),

			(uint8_t)((sleep_time >> 16) & 0xFF),
			(uint8_t)((sleep_time >>  8) & 0xFF),
			(uint8_t)((sleep_time      ) & 0xFF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_RXDUTYCYCLE, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_cad(sx126x_api_t * api)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_CAD, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_tx_continous_wave(sx126x_api_t * api)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_TXCONTINUOUSWAVE, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_tx_continous_preamble(sx126x_api_t * api)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_TXCONTINUOUSPREAMBLE, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_regulator_mode(sx126x_api_t * api, sx126x_regulator_t mode)
{
	int rc;

	uint8_t arg = mode;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_REGULATORMODE, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_calibrate(sx126x_api_t * api, uint8_t calib_flags)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_CALIBRATE, &calib_flags, sizeof(calib_flags));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_calibrate_image(sx126x_api_t * api, const uint8_t * args)
{
	int rc;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_CALIBRATEIMAGE, args, SX126X_API_CALIBRATE_IMAGE_ARG_LEN);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_pa_config(sx126x_api_t * api,
		uint8_t pa_duty_cycle, uint8_t hp_max, uint8_t device_sel, uint8_t pa_lut
)
{
	int rc;
	const uint8_t args[4] = { pa_duty_cycle, hp_max, device_sel, pa_lut };

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_PACONFIG, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return rc;
}


int sx126x_api_set_rxtx_fallback_mode(sx126x_api_t * api, sx126x_fallback_mode_t mode)
{
	int rc;
	const uint8_t arg = mode;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_RXTXFALLBACKMODE, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_dio_irq_params(sx126x_api_t * api,
		uint16_t global_irq_mask, uint16_t dio1_irq_mask, uint16_t dio2_irq_mask, uint16_t dio3_irq_mask
)
{
	int rc;
	const uint8_t args[8] = {
			(uint8_t)((global_irq_mask >> 8) & 0x00FF),
			(uint8_t)((global_irq_mask     ) & 0x00FF),

			(uint8_t)((dio1_irq_mask >> 8) & 0x00FF),
			(uint8_t)((dio1_irq_mask     ) & 0x00FF),

			(uint8_t)((dio2_irq_mask >> 8) & 0x00FF),
			(uint8_t)((dio2_irq_mask     ) & 0x00FF),

			(uint8_t)((dio3_irq_mask >> 8) & 0x00FF),
			(uint8_t)((dio3_irq_mask     ) & 0x00FF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_CFG_DIOIRQ, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_get_irq_status(sx126x_api_t * api, uint16_t * irq_status)
{
	int rc;
	uint8_t args[2];

	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_IRQSTATUS, NULL, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	*irq_status = (uint16_t)args[0] << 8 | (uint16_t)args[1];
	return 0;
}


int sx126x_api_clear_irq_status(sx126x_api_t * api, uint16_t clear_mask)
{
	int rc;
	const uint8_t args[2] = {
			(uint8_t)((clear_mask >> 8) & 0x00FF),
			(uint8_t)((clear_mask     ) & 0x00FF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_CLR_IRQSTATUS, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_dio2_as_rf_switch(sx126x_api_t * api, bool enable)
{
	int rc;
	const uint8_t arg = enable ? 0x01 : 0x00;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_RFSWITCHMODE, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_dio3_as_tcxo_ctl(sx126x_api_t * api, sx126x_tcxo_v_t tcxo_voltage, uint32_t timeout_ms)
{
	int rc;
	const uint32_t timeout = SX126X_MS_TO_TIME_U(timeout_ms);

	uint8_t args[4] = {
			tcxo_voltage & 0x07,
			(uint8_t)((timeout >> 16) & 0xFF),
			(uint8_t)((timeout >>  8) & 0xFF),
			(uint8_t)((timeout      ) & 0xFF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_TCXOMODE, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_rf_frequency(sx126x_api_t * api, uint32_t frequency)
{
	int rc;
	const uint32_t freq_arg = (uint32_t)(frequency / ((double)SX126X_F_XTAL / (double)SX126X_2_POW_25));
	const uint8_t args[4] = {
			(uint8_t)((freq_arg >> 24) & 0xFF),
			(uint8_t)((freq_arg >> 16) & 0xFF),
			(uint8_t)((freq_arg >>  8) & 0xFF),
			(uint8_t)((freq_arg      ) & 0xFF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_RFFREQUENCY, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_packet_type(sx126x_api_t * api, sx126x_packet_type_t packet_type)
{
	int rc;
	const uint8_t arg = packet_type;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_PACKETTYPE, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_get_packet_type(sx126x_api_t * api, sx126x_packet_type_t * packet_type)
{
	int rc;
	uint8_t arg = 0;

	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_SET_PACKETTYPE, NULL, &arg, sizeof(arg));
	SX126X_RETURN_IF_NONZERO(rc);

	switch (arg)
	{
	case SX126X_PACKET_TYPE_GFSK:
	case SX126X_PACKET_TYPE_LORA:
		break;

	default:
		return SX126x_ERROR_UNEXPECTED_VALUE;
	}

	*packet_type = (sx126x_packet_type_t)arg;
	return 0;
}


int sx126x_api_set_tx_params(sx126x_api_t * api, int8_t power, sx126x_pa_ramp_time_t pa_ramp_time)
{
	int rc;
	// TODO: В примере очень много разного текста. Очень много. Нужно внимательно посмотреть

	const uint8_t args[2] = { power, pa_ramp_time };
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_TXPARAMS, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_modulation_params(sx126x_api_t * api, const uint8_t * raw_args, uint16_t raw_args_size)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_MODULATIONPARAMS, raw_args, raw_args_size);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_gfsk_modulation_params(sx126x_api_t * api, const sx126x_gfsk_modulation_params_t * params)
{
	int rc;
	const uint32_t scaled_bit_rate = (uint32_t)(32.0 * ((double)SX126X_F_XTAL / (double)params->bit_rate));
	const uint32_t fdev_scaled =
			(uint32_t)((double)params->f_dev / ((double)SX126X_F_XTAL / (double)SX126X_2_POW_25))
	;

	const uint8_t args[8] = {
			/* 1 */(scaled_bit_rate >> 16) & 0xFF,
			/* 2 */(scaled_bit_rate >>  8) & 0xFF,
			/* 3 */(scaled_bit_rate      ) & 0xFF,
			/* 4 */params->pulse_shape,
			/* 5 */params->bandwidth,
			/* 6 */(fdev_scaled >> 16 ) & 0xFF,
			/* 7 */(fdev_scaled >>  8 ) & 0xFF,
			/* 8 */(fdev_scaled       ) & 0xFF
	};

	rc = sx126x_api_set_modulation_params(api, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_lora_modulation_params(sx126x_api_t * api, const sx126x_lora_modulation_params_t * params)
{
	int rc;
	const uint8_t args[4] = {
			params->spreading_factor,
			params->bandwidth,
			params->coding_rate,
			params->ldr_optimizations ? 0x01 : 0x00
	};

	rc = sx126x_api_set_modulation_params(api, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_packet_params(sx126x_api_t * api, const uint8_t * raw_args, uint16_t raw_args_size)
{
	int rc;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_PACKETPARAMS, raw_args, raw_args_size);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_gfsk_packet_params(sx126x_api_t * api, const sx126x_gfsk_packet_params_t * params)
{
	int rc;
	const uint8_t args[9] = {
			/* 1 */(params->preamble_length >> 8) & 0xFF,
			/* 2 */(params->preamble_length     ) & 0xFF,
			/* 3 */params->preamble_detector,
			/* 4 */params->sync_word_length,
			/* 5 */params->adress_comp,
			/* 6 */params->explicit_header ? 0x01 : 0x00,
			/* 7 */params->payload_length,
			/* 8 */params->crc_length,
			/* 9 */params->enable_whitening ? 0x01 : 0x00
	};

	rc = sx126x_api_set_packet_params(api, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_lora_packet_params(sx126x_api_t * api, const sx126x_lora_packet_params_t * params)
{
	int rc;
	const uint8_t args[6] = {
			/* 1 */(params->preamble_length >> 8) & 0xFF,
			/* 2 */(params->preamble_length     ) & 0xFF,
			/* 3 */params->explicit_header ? 0x00 : 0x01,
			/* 4 */params->payload_length,
			/* 5 */params->use_crc ? 0x01 : 0x00,
			/* 6 */params->invert_iq ? 0x01 : 0x00,
	};

	rc = sx126x_api_set_packet_params(api, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_cad_params(sx126x_api_t * api,
		sx126x_cad_length_t cad_len, uint8_t cad_peak, uint8_t cad_min,
		sx126x_cad_exit_mode_t exit_mode, uint32_t cad_timeout_ms
)
{
	int rc;
	const uint32_t cad_timeout = SX126X_MS_TO_TIME_U(cad_timeout_ms);
	uint8_t args[7] = {
			/* 1 */(uint8_t)cad_len,
			/* 2 */cad_peak,
			/* 3 */cad_min,
			/* 4 */(uint8_t)exit_mode,
			/* 5 */(uint8_t)((cad_timeout >> 16) & 0xFF),
			/* 6 */(uint8_t)((cad_timeout >>  8) & 0xFF),
			/* 7 */(uint8_t)((cad_timeout      ) & 0xFF)
	};

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_CADPARAMS, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_buffer_base_address(sx126x_api_t * api, uint8_t tx_base_addr, uint8_t rx_base_addr)
{
	int rc;
	const uint8_t args[2] = { tx_base_addr, rx_base_addr };

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_BUFFERBASEADDRES, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_set_lora_symb_num_timeout(sx126x_api_t * api, uint8_t symb_num)
{
	int rc;

	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_SET_LORASYMBTIMEOUT, &symb_num, sizeof(symb_num));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_get_status(sx126x_api_t * api, sx126x_status_t * status)
{
	int rc;

	uint8_t status_value;
	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_STATUS, &status_value, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	status->value = status_value;
	return 0;
}


int sx126x_api_get_rx_buffer_status(sx126x_api_t * api, uint8_t * payload_size, uint8_t * payload_offset)
{
	int rc;

	uint8_t data[2];
	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_RXBUFFERSTATUS, NULL, data, sizeof(data));
	SX126X_RETURN_IF_NONZERO(rc);

	*payload_size = data[0];
	*payload_offset = data[1];
	return 0;
}


int sx126x_api_get_packet_status(sx126x_api_t * api, uint8_t * raw_data_buffer)
{
	int rc;

	rc = sx126x_brd_cmd_read(
			api->board, SX126X_CMD_GET_PACKETSTATUS, NULL, raw_data_buffer, SX126X_API_PACKET_STATUS_ARG_LEN
	);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_get_gfsk_packet_status(sx126x_api_t * api, sx126x_gfsk_packet_status_t * status)
{
	int rc;
	uint8_t raw[SX126X_API_PACKET_STATUS_ARG_LEN] = {0};

	rc = sx126x_api_get_packet_status(api, raw);
	SX126X_RETURN_IF_NONZERO(rc);

	status->rx_status = raw[0];
	status->rssi_sync = -raw[1] >> 1;
	status->rssi_avg = raw[2] >> 1;
	return 0;
}


int sx126x_api_get_lora_packet_status(sx126x_api_t * api, sx126x_lora_packet_status_t * status)
{
	int rc;
	uint8_t raw[SX126X_API_PACKET_STATUS_ARG_LEN];

	rc = sx126x_api_get_packet_status(api, raw);
	SX126X_RETURN_IF_NONZERO(rc);

	status->rssi_pkt = -1*(raw[0] >> 1);
	status->snr_pkt = (raw[1] < 128) ? raw[1] >> 2 : (raw[1] - 256) >> 2;
	status->signal_rssi_pkt = -1*(raw[2] >> 1);
	return 0;
}


int sx126x_api_get_rssi_inst(sx126x_api_t * api, int8_t * rssi)
{
	int rc;
	uint8_t args[1];

	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_RSSIINST, NULL, args, sizeof(args));
	SX126X_RETURN_IF_NONZERO(rc);

	*rssi = -args[0] >> 1;
	return 0;
}


int sx126x_api_get_stats(sx126x_api_t * api, sx126x_stats_t * stats)
{
	int rc;

	uint8_t raw[6];
	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_STATS, NULL, raw, sizeof(raw));
	SX126X_RETURN_IF_NONZERO(rc);

	stats->pkt_received =  ((uint16_t)raw[0] << 8) | raw[1];
	stats->crc_errors =    ((uint16_t)raw[2] << 8) | raw[3];
	stats->hdr_errors =    ((uint16_t)raw[4] << 8) | raw[5];
	return 0;
}


int sx126x_api_reset_stats(sx126x_api_t * api)
{
	int rc;
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_RESET_STATS, NULL, 0);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_api_get_device_errors(sx126x_api_t * api, uint16_t * device_errors)
{
	int rc;
	uint8_t raw[2];
	rc = sx126x_brd_cmd_read(api->board, SX126X_CMD_GET_ERROR, NULL, raw, sizeof(raw));
	SX126X_RETURN_IF_NONZERO(rc);

	*device_errors = ((uint16_t)raw[0] << 8) | raw[1];
	return 0;
}


int sx126x_api_clear_device_errors(sx126x_api_t * api)
{
	int rc;
	uint8_t dummy[1] = { 0x00 };
	rc = sx126x_brd_cmd_write(api->board, SX126X_CMD_CLR_ERROR, dummy, sizeof(dummy));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}
