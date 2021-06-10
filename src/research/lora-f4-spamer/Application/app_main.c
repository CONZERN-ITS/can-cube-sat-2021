
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <stm32f4xx_hal.h>

#include "sx126x_drv.h"

#define APP_PACKET_SIZE 200


static sx126x_drv_t _radio;
static uint8_t _pbuff[255] = {0};
static uint8_t _psize;


static int _push_packet(sx126x_drv_t * radio)
{
	int rc;
	static uint8_t serial = 0;
	for (size_t i = 0; i < APP_PACKET_SIZE; i++)
		_pbuff[i] = serial++;

	_psize = APP_PACKET_SIZE;

	rc = sx126x_drv_payload_write(radio, _pbuff, _psize);
	if (0 != rc) return rc;

	return 0;
}


static int _fetch_packet(sx126x_drv_t * radio)
{
	int rc;

	rc = sx126x_drv_payload_rx_size(radio, &_psize);
	if (0 != rc) return rc;

	// К сожалению яхз как вытащить фактическую цифру из радио
	_psize = APP_PACKET_SIZE;

	rc = sx126x_drv_payload_read(radio, _pbuff, _psize);
	if (0 != rc) return rc;

	printf("got packet %d\n", (int)_pbuff[0]);
	return 0;
}


static void _event_handler(sx126x_drv_t * drv, void * user_arg, sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg)
{
	int rc = 0;
	uint32_t now = HAL_GetTick();

	switch (kind)
	{
	case SX126X_EVTKIND_TX_DONE:
		printf("%d tx done\n", (int)now);
		if (arg->tx_done.timed_out)
			printf("tx timed out!\n");

		rc = sx126x_drv_mode_standby(drv);
		if (0 != rc) break;

		// Уходим в цикл RX
		rc = sx126x_drv_mode_rx(drv, 100);
		break;

	case SX126X_EVTKIND_RX_DONE:
		if (!arg->rx_done.timed_out)
		{
			sx126x_lora_packet_status_t packet_status;
			rc = sx126x_drv_lora_packet_status(drv, &packet_status);
			if (0 != rc)
				printf("unable to fetch packet status: %d\n", rc);

			printf(
				"%d, rx done. crc: %s, prssi %d, snr: %d, srssi: %d\n",
				(int)now,
				arg->rx_done.crc_valid ? "true" : "false",
				packet_status.rssi_pkt,
				packet_status.signal_rssi_pkt,
				packet_status.snr_pkt
			);
			rc = _fetch_packet(drv);
			if (0 != rc) break;

		}
		rc = sx126x_drv_mode_standby(drv);
		if (0 != rc) break;

		rc = _push_packet(drv);
		if (0 != rc) break;

		rc = sx126x_drv_mode_tx(drv, 0);
		if (0 != rc) break;
		break;

	case SX126X_EVTKIND_CAD_DONE:
		printf("%d cad done?\n", (int)now);
		break;

	default:
		printf("unknown event: %d\n", kind);
	};

	if (0 != rc)
		printf("event handler error: %d\n", rc);
}


static void print_status(void)
{
	sx126x_status_t status;
	int rc = sx126x_api_get_status(&_radio.api, &status);
	printf("status read; rc=%d, cmd_status=%d, dev_status=%d\n",
			rc, (int)status.bits.cmd_status, (int)status.bits.chip_mode
	);
}


int app_main(void)
{
	int rc;

	const sx126x_drv_basic_cfg_t basic_cfg = {
			.use_dio3_for_tcxo = true,
			.tcxo_v = SX126X_TCXO_CTRL_1_8V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.standby_mode = SX126X_STANDBY_XOSC
	};

	const sx126x_drv_lora_modem_cfg_t modem_cfg = {
			// Параметры усилителей и частот
			.frequency = 435125*1000,
			.pa_ramp_time = SX126X_PA_RAMP_1700_US,
			.pa_power = 10,
			.lna_boost = false,

			// Параметры пакетирования
			.spreading_factor = SX126X_LORA_SF_7,
			.bandwidth = SX126X_LORA_BW_250,
			.coding_rate = SX126X_LORA_CR_4_8,
			.ldr_optimizations = false,
	};

	const sx126x_drv_lora_packet_cfg_t packet_cfg = {
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,
			.preamble_length = 16,
			.explicit_header = true,
			.payload_length = APP_PACKET_SIZE,
			.use_crc = true,
	};

	const sx126x_drv_cad_cfg_t cad_cfg = {
			.cad_len = SX126X_LORA_CAD_04_SYMBOL,
			.cad_min = 10,
			.cad_peak = 28,
			.exit_mode = SX126X_LORA_CAD_RX,
	};

	const sx126x_drv_lora_rx_timeout_cfg_t rx_timeout_cfg = {
			.stop_timer_on_preamble = false,
			.lora_symb_timeout = 0
	};

	rc = sx126x_drv_ctor(&_radio, NULL);
	printf("ctor: %d\n", rc);
	print_status();

	rc = sx126x_drv_set_event_handler(&_radio, _event_handler, NULL);
	printf("register_event_handler: %d\n", rc);
	print_status();

	rc = sx126x_drv_reset(&_radio);
	printf("reset: %d\n", rc);
	print_status();

	rc = sx126x_drv_configure_basic(&_radio, &basic_cfg);
	printf("configure_basic: %d\n", rc);
	print_status();

	rc = sx126x_drv_configure_lora_modem(&_radio, &modem_cfg);
	printf("configure_lora_modem: %d\n", rc);
	print_status();

	rc = sx126x_drv_configure_lora_packet(&_radio, &packet_cfg);
	printf("configure_lora_packet: %d\n", rc);
	print_status();

	rc = sx126x_drv_configure_lora_cad(&_radio, &cad_cfg);
	printf("configure_cad: %d\n", rc);
	print_status();

	rc = sx126x_drv_configure_lora_rx_timeout(&_radio, &rx_timeout_cfg);
	printf("configure_timeout: %d\n", rc);
	print_status();

	rc = sx126x_drv_mode_standby(&_radio);
	printf("went to standby: %d\n", rc);
	print_status();

	// Начинаем с того, что начинаем приём
	rc = _push_packet(&_radio);
	printf("first packet pushed: %d\n", rc);
	print_status();

	rc = sx126x_drv_mode_tx(&_radio, 0);
	printf("first tx: %d\n", rc);
	print_status();

	while(1)
	{
		rc = sx126x_drv_poll_irq(&_radio);
		if (rc)
			printf("poll error: %d\n", rc);

		HAL_Delay(50);
	}

	return 0;
}
