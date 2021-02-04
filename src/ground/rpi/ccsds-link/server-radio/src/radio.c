#include "radio.h"

#include <string.h>


static void _event_handler(
		sx126x_drv_t * drv,
		void * user_arg,
		sx126x_evt_kind_t kind,
		const sx126x_evt_arg_t * arg
)
{

}


int radio_init(radio_t * radio)
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
			.payload_length = RADIO_PACKET_SIZE,
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


	rc = sx126x_drv_ctor(&radio->dev, NULL);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_register_event_handler(&radio->dev, _event_handler, NULL);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_reset(&radio->dev);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_mode_standby_rc(&radio->dev);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_configure_basic(&radio->dev, &basic_cfg);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_configure_lora_modem(&radio->dev, &modem_cfg);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_configure_lora_packet(&radio->dev, &packet_cfg);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_configure_lora_cad(&radio->dev, &cad_cfg);
	if (0 != rc)
		return rc;

	rc = sx126x_drv_configure_lora_rx_timeout(&radio->dev, &rx_timeout_cfg);
	if (0 != rc)
		return rc;

	return 0;
}


void radio_deinit(radio_t * radio)
{
	sx126x_drv_dtor(&radio->dev);
}
