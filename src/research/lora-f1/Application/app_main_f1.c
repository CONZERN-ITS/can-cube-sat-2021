
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <stm32f1xx_hal.h>

#include "sx126x_drv.h"


static sx126x_dev_t _radio;
static uint8_t _packet_no = 0;
static uint32_t last_tick = 0;

bool _waiting_reply = false;


static uint8_t packet_provider(void * arg, uint8_t * dest, uint8_t dest_size, int * flags, int * cookie)
{
	/*
	if (HAL_GetTick() - last_tick < 6000)
		return 0;
	*/

	memset(dest, _packet_no, dest_size);
	_packet_no++;
	_waiting_reply = true;
	last_tick = HAL_GetTick();

	*flags = 0;
	*cookie = dest[0];

	return dest_size;
}


static void packet_acker(void * arg, int flags, int cookie)
{
	printf("sent ping %d\n", cookie);
}


static void packet_consumer(void * arg, const uint8_t * src, uint8_t packet_size, int flags)
{
	printf("got pong %d\n", (int)src[0]);
}


int app_main(void)
{
	int rc;

	const sx126x_drv_chip_cfg_t chip_cfg = {
			.use_dio3_for_tcxo = true,
			.tcxo_v = SX126X_TCXO_CTRL_1_6V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.rxtx_fallback_mode = SX126X_FALLBACK_FS,
	};

	const sx126x_drv_lora_cfg_t lora_cfg = {
			.frequency = 434*1000*1000,
			.pa_ramp_time = SX126X_PA_RAMP_3400_US,
			.pa_power = 10,

			// Параметры модуляции
			.spreading_factor = SX126X_LORA_SF_12,
			.bandwidth = SX126X_LORA_BW_500,
			.coding_rate = SX126X_LORA_CR_4_5,
			.ldr_optimizations = false,

			// Параметры пакетизации
			.preamble_length = 24,
			.explicit_header = true,
			.payload_length = 200,
			.use_crc = true,
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,

			// Всякие прочие параметры
			.boost_rx_lna = true,

			// CAD параметры
			.cad_len = SX126X_LORA_CAD_16_SYMBOLS,
			.cad_min = 10,
			.cad_peak = 28
	};

	sx126x_drv_callback_cfg_t callback_cfg = {
			.cb_user_arg = NULL,
			.onrx_callback = packet_consumer,
			.ontx_callback = packet_provider,
			.ontxcplt_callback = packet_acker,
	};

	rc = sx126x_drv_ctor(&_radio, NULL);
	printf("ctor: %d\n", rc);

	rc = sx126x_drv_mode_standby_rc(&_radio);
	printf("standy_rc: %d\n", rc);

	rc = sx126x_drv_configure_chip(&_radio, &chip_cfg);
	printf("configure_chip: %d\n", rc);

	rc = sx126x_drv_configure_lora(&_radio, &lora_cfg);
	printf("configure_lora: %d\n", rc);

	rc = sx126x_drv_configure_callbacks(&_radio, &callback_cfg);
	printf("configure_callbacks: %d\n", rc);

	rc = sx126x_drv_start(&_radio);
	printf("drv start: %d\n", rc);

	while(1)
	{
		rc = sx126x_drv_poll(&_radio);
		if (rc)
			printf("poll error: %d\n", rc);

		//HAL_Delay(50);
	}


	return 0;
}
