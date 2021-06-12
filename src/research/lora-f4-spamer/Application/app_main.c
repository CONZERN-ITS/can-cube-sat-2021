
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <stm32f4xx_hal.h>

#include "sx126x_drv.h"

#define APP_PACKET_SIZE 200


static sx126x_drv_t _radio;


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
			.frequency = 434125*1000,
			.pa_ramp_time = SX126X_PA_RAMP_1700_US,
			.pa_power = 10,
			.lna_boost = true,

			// Параметры пакетирования
			.spreading_factor = SX126X_LORA_SF_7,
			.bandwidth = SX126X_LORA_BW_250,
			.coding_rate = SX126X_LORA_CR_4_8,
			.ldr_optimizations = false,
	};

	const sx126x_drv_lora_packet_cfg_t packet_cfg = {
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,
			.preamble_length = 48,
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

	uint8_t packet[APP_PACKET_SIZE];
	uint8_t i;
	for (i = 0; ; i++)
	{
		memset(packet, 0x00, sizeof(packet));
		sprintf((char*)packet, "hello lora!");
		//memset(packet, i, sizeof(packet));
		rc = sx126x_drv_payload_write(&_radio, packet, APP_PACKET_SIZE);
		printf("payload write error: %d\n", rc);

		sx126x_drv_evt_t event;
		rc = sx126x_drv_tx_blocking(&_radio, 10*1000, 15*1000, &event);
		printf("tx blocking: rc: %d, ek: %d, to: %d\n", rc, (int)event.kind, (int)event.arg.tx_done.timed_out);

		rc = sx126x_drv_rx_blocking(&_radio, 1*1000, 2*1000, &event);
		printf(
				"rx blocking rc: %d; ek: %d; to: %d; crc: %d\n",
				rc, (int)event.kind, (int)event.arg.rx_done.timed_out,
				(int)event.arg.rx_done.crc_valid
		);

		if (event.kind == SX126X_EVTKIND_RX_DONE && !event.arg.rx_done.timed_out)
		{
			printf("rx done!\n");
			uint8_t packet_size;
			rc = sx126x_drv_payload_rx_size(&_radio, &packet_size);
			printf("payload size rc = %d; size = %d\n", (int)rc, (int)packet_size);

			rc = sx126x_drv_payload_read(&_radio, packet, APP_PACKET_SIZE);
			printf("payload read = %d\n", (int)packet_size);
		}


		HAL_Delay(100);
	}

	return 0;
}
