#include <unistd.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sx126x_drv.h>


#define APP_PACKET_SIZE 200


static sx126x_drv_t _radio;
static uint8_t _pbuff[255] = {0};


static void _gen_packet(uint8_t * where, uint8_t where_size)
{
	static uint8_t serial = 0;
	memset(where, serial, where_size);
	serial++;
}


static int _send_packet(sx126x_drv_t * radio, uint8_t psize)
{
	int rc;
	_gen_packet(_pbuff, psize);

	rc = sx126x_drv_payload_write(radio, _pbuff, psize);
	if (0 != rc) return rc;

	rc = sx126x_drv_mode_tx(radio, 0);
	if (0 != rc) return rc;

	printf("sent packet %d\n", (int)_pbuff[0]);
	return rc;
}


static int _fetch_packet(sx126x_drv_t * drv)
{
	int rc;
	uint8_t rx_size;

	rc = sx126x_drv_payload_rx_size(drv, &rx_size);
	if (0 != rc) return rc;

	rc = sx126x_drv_payload_read(drv, _pbuff, rx_size);
	if (0 != rc) return rc;

	printf("got packet %d\n", (int)_pbuff[0]);
	return 0;
}


static void _event_handler(sx126x_drv_t * drv, void * user_arg, sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg)
{
	int rc;

	switch (kind)
	{
	case SX126X_EVTKIND_RX_DONE:
		if (arg->rx_done.timed_out)
		{
			// Если эфир свободен
			// Отправляем свой пакет
			rc = _send_packet(drv, APP_PACKET_SIZE);
			if (0 != rc) break;
		}
		else
		{
			// Если нет - достаем пакет и идем слушать новый
			rc = _fetch_packet(drv);
			if (0 != rc) break;

			rc = sx126x_drv_mode_rx(drv, 0);
			if (0 != rc) break;
		}
		break;

	case SX126X_EVTKIND_TX_DONE:
		if (arg->tx_done.timed_out)
		{
			printf("tx timed out!\n");
		}

		// Начинаем слушать
		rc = sx126x_drv_mode_rx(drv, 0);
		if (0 != rc) break;

		break;

	case SX126X_EVTKIND_CAD_DONE:
		printf("cad done?\n");
		break;

	default:
		printf("unknown event: %d\n", kind);
	};


	if (0 != rc)
	{
		printf("event handler error: %d\n", rc);
	}
}


int main(void)
{
	int rc;

	const sx126x_drv_basic_cfg_t basic_cfg = {
			.use_dio3_for_tcxo = true,
			.tcxo_v = SX126X_TCXO_CTRL_1_6V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.standby_mode = SX126X_STANDBY_XOSC
	};

	const sx126x_drv_lora_modem_cfg_t modem_cfg = {
			// Параметры усилителей и частот
			.frequency = 434*1000*1000,
			.pa_ramp_time = SX126X_PA_RAMP_3400_US,
			.pa_power = 10,
			.lna_boost = true,

			// Параметры пакетирования
			.spreading_factor = SX126X_LORA_SF_12,
			.bandwidth = SX126X_LORA_BW_250,
			.coding_rate = SX126X_LORA_CR_4_5,
			.ldr_optimizations = true,
	};

	const sx126x_drv_lora_packet_cfg_t packet_cfg = {
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,
			.preamble_length = 24,
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
			.stop_timer_on_preable = false,
			.lora_symb_timeout = 50
	};


	rc = sx126x_drv_ctor(&_radio, NULL);
	printf("ctor: %d\n", rc);

	rc = sx126x_drv_register_event_handler(&_radio, _event_handler, NULL);
	printf("register_event_handler: %d\n", rc);

	rc = sx126x_drv_reset(&_radio);
	printf("reset: %d\n", rc);

	rc = sx126x_drv_mode_standby_rc(&_radio);
	printf("standy_rc: %d\n", rc);

	rc = sx126x_drv_configure_basic(&_radio, &basic_cfg);
	printf("configure_basic: %d\n", rc);

	rc = sx126x_drv_configure_lora_modem(&_radio, &modem_cfg);
	printf("configure_lora_modem: %d\n", rc);

	rc = sx126x_drv_configure_lora_packet(&_radio, &packet_cfg);
	printf("configure_lora_packet: %d\n", rc);

	rc = sx126x_drv_configure_lora_cad(&_radio, &cad_cfg);
	printf("configure_cad: %d\n", rc);

	rc = sx126x_drv_configure_lora_rx_timeout(&_radio, &rx_timeout_cfg);
	printf("configure_timeout: %d\n", rc);

	// Начинаем с отправки своего пакета
	rc = _send_packet(&_radio, APP_PACKET_SIZE);
	printf("first tx: %d\n", rc);

	while(1)
	{
		rc = sx126x_drv_poll(&_radio);
		if (rc)
			printf("poll error: %d\n", rc);

		usleep(500*1000);
		printf("poll cycle_compete\n");
	}


	return 0;
}
