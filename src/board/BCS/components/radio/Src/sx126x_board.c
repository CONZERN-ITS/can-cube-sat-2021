/*
 * sx126x_impl.c
 *
 *  Created on: Mar 27, 2021
 *      Author: sereshotes
 */
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common_internal.h"

#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hal/spi_hal.h"

#include "pinout_cfg.h"

#include "sx126x_board.h"

struct sx126x_board_t {
	spi_device_handle_t hspi;

};

static sx126x_board_t _brd;


int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg) {
	*brd = &_brd;
	(void)user_arg;
	esp_err_t ret = 0;

	spi_device_interface_config_t devcfg = {
		.command_bits = 0,
		.address_bits = 0,
		.dummy_bits = 0,
		.flags = 0,
		.clock_speed_hz = 10000,
		.mode = 0,
		.spics_io_num = -1,
		.queue_size = 30,
		.pre_cb = 0,
	};
	ret = spi_bus_add_device(ITS_SPISR_PORT, &devcfg, &(*brd)->hspi);
	if (ret != ESP_OK) {
		return ret;
	}

	gpio_set_level(ITS_PIN_RADIO_RESET, 1);
	gpio_set_level(ITS_PIN_RADIO_TX_EN, 0);
	gpio_set_level(ITS_PIN_RADIO_RX_EN, 0);
	gpio_set_level(ITS_PIN_SPISR_CS_RA, 1);

	return 0;
}
void sx126x_brd_dtor(sx126x_board_t * brd) {
	(void) brd;
}

int sx126x_brd_get_time(sx126x_board_t * brd, uint32_t * value) {
	return esp_timer_get_time() / 1000; //ms or us?
}

int sx126x_brd_get_chip_type(sx126x_board_t * brd, sx126x_chip_type_t * chip_type) {
	// У нас используется именно такой
	*chip_type = SX126X_CHIPTYPE_SX1268;
	return 0;
}

int sx126x_brd_reset(sx126x_board_t * brd) {
	gpio_set_level(ITS_PIN_SPISR_CS_RA, 0);
	vTaskDelay(50 / portTICK_RATE_MS);
	gpio_set_level(ITS_PIN_SPISR_CS_RA, 1);
	return 0;
}

int sx126x_brd_wait_on_busy(sx126x_board_t * brd) {
	while (gpio_get_level(ITS_PIN_RADIO_BUSY) != 0) {
		vTaskDelay(1);
	}
	return 0;
}

int sx126x_brd_cleanup_irq(sx126x_board_t * brd) {
	return 0;
}

int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode) {
	gpio_set_level(ITS_PIN_RADIO_TX_EN, 0);
	gpio_set_level(ITS_PIN_RADIO_RX_EN, 0);

	gpio_set_level(ITS_PIN_RADIO_TX_EN, mode == SX126X_ANTENNA_TX);
	gpio_set_level(ITS_PIN_RADIO_RX_EN, mode == SX126X_ANTENNA_RX);
	return 0;
}

static int _write_data(sx126x_board_t * brd, const uint8_t * data, uint16_t data_size) {
	ESP_LOGD("radio", "spi write %d", data_size);
	spi_transaction_t tran = {0};
	tran.tx_buffer = data;
	tran.length = 8 * data_size;
	return spi_device_polling_transmit(brd->hspi, &tran);
}

static int _read_data(sx126x_board_t * brd, uint8_t * data, uint16_t data_size) {
	ESP_LOGD("radio", "spi read  %d", data_size);
	memset(data, 0xFF, data_size);
	spi_transaction_t tran = {0};
	tran.rx_buffer = data;
	tran.rxlength = 8 * data_size;
	tran.length = 8 * data_size;
	tran.tx_buffer = data;
	return spi_device_polling_transmit(brd->hspi, &tran);
}

static void _start_trans(sx126x_board_t * brd) {
	spi_device_acquire_bus(brd->hspi, portMAX_DELAY);
	gpio_set_level(ITS_PIN_SPISR_CS_RA, 0);
}
static void _end_trans(sx126x_board_t * brd) {
	gpio_set_level(ITS_PIN_SPISR_CS_RA, 1);
	spi_device_release_bus(brd->hspi);
}

int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size) {
	ESP_LOGD("radio", "spi cmdw  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, &cmd_code, 1);
	_write_data(brd, args, args_size);
	_end_trans(brd);
	return 0;
}
int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status_, uint8_t * data, uint16_t data_size) {
	uint8_t dummy_status;
	uint8_t * status = status_ ? status_ : &dummy_status;
	*status = 0xFF; // Гоним единички на TX

	ESP_LOGD("radio", "spi cmdr  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, &cmd_code, 1);
	_read_data(brd, status, 1);
	_read_data(brd, data, data_size);
	_end_trans(brd);

	return 0;
}

int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size) {
	const uint8_t cmd_code = SX126X_CMD_WRITE_REGISTER;

	addr = ((addr >> 8) & 0xFF) | ((addr >> 0) & 0xFF);

	ESP_LOGD("radio", "spi cmdw  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, (uint8_t * const) &cmd_code, 1);
	_write_data(brd, (uint8_t * const) &addr, 2);
	_write_data(brd, data, data_size);
	_end_trans(brd);

	return 0;
}
int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size) {
	const uint8_t cmd_code = SX126X_CMD_READ_REGISTER;

	uint8_t status = 0xFF;
	ESP_LOGD("radio", "spi cmdr  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, (uint8_t * const) &cmd_code, 1);
	_read_data(brd, &status, 1);
	_read_data(brd, data, data_size);
	_end_trans(brd);
	return 0;
}

int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size) {
	const uint8_t cmd_code = SX126X_CMD_WRITE_BUFFER;

	ESP_LOGD("radio", "spi cmdw  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, (uint8_t * const) &cmd_code, 1);
	_write_data(brd, &offset, 1);

	while (data_size > 0) {
		uint8_t to_send = data_size;
		if (to_send > 20) {
			to_send = 20;
		}
		_write_data(brd, data, to_send);
		data += to_send;
		data_size -= to_send;
	}
	_end_trans(brd);
	return 0;
}
int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size) {
	const uint8_t cmd_code = SX126X_CMD_READ_BUFFER;

	uint8_t status = 0xFF;
	ESP_LOGD("radio", "spi cmdr  0x%02X", cmd_code);
	_start_trans(brd);
	_write_data(brd, (uint8_t * const) &cmd_code, 1);
	_read_data(brd, &status, 1);
	_read_data(brd, data, data_size);
	_end_trans(brd);
	return 0;
}



