/*
 * radio_simple_api.c
 *
 *  Created on: Mar 27, 2021
 *      Author: sereshotes
 */
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "init_helper.h"
#include "router.h"

#include "radio.h"


static int64_t _timespec_diff_ms(const struct timespec * left, const struct timespec * right)
{
	return (left->tv_sec - right->tv_sec) * 1000 - (left->tv_nsec - right->tv_nsec) / (1000 * 1000);
}




static void _radio_event_handler(sx126x_drv_t * drv, void * user_arg,
		sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg
);


static void _radio_deinit(server_t * server)
{
	sx126x_drv_t * const radio = &server->dev;

	// резетим радио на последок
	// Даже если ничего не получается
	sx126x_drv_reset(radio);
	// Деструктим дескриптор
	sx126x_drv_dtor(radio);
}


static int _radio_init(server_t * server)
{
	sx126x_drv_t * const radio = &server->dev;
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
			.frequency = 433800*1000,
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
			.lora_symb_timeout = 100
	};


	rc = sx126x_drv_ctor(radio, NULL);

	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_register_event_handler(radio, _radio_event_handler, server);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_reset(radio);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_mode_standby_rc(radio);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_configure_basic(radio, &basic_cfg);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_configure_lora_modem(radio, &modem_cfg);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_configure_lora_packet(radio, &packet_cfg);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_configure_lora_cad(radio, &cad_cfg);
	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_configure_lora_rx_timeout(radio, &rx_timeout_cfg);
	if (0 != rc)
		goto bad_exit;

	return 0;

bad_exit:
	log_error("BAD INIT");
	_radio_deinit(server);
	return -1;
}


bool _radio_go_tx_if_any(server_t * server)
{
	int rc;
	bool retval = true;

	if (0 == server->tx_cookie_wait)
		return false;

	log_trace("going tx");
	rc = sx126x_drv_payload_write(&server->dev, server->tx_buffer, server->tx_buffer_size);
	if (0 != rc)
	{
		log_error("unable to write tx payload to radio: %d. Dropping frame", rc);
		server->tx_cookie_dropped = server->tx_cookie_wait;
		retval = false;
		goto end;
	}

	rc = sx126x_drv_mode_tx(&server->dev, server->tx_timeout_ms);
	if (0 != rc)
	{
		log_error("unable to switch radio to tx mode: %d. Dropping frame", rc);
		server->tx_cookie_dropped = server->tx_cookie_wait;
		retval = false;
		goto end;
	}

	server->tx_cookie_in_progress = server->tx_cookie_wait;


end:
	server->tx_cookie_wait = 0;
	server->tx_cookies_updated = true;
	return retval;
}


static void _radio_go_rx(server_t * server)
{
	int rc = sx126x_drv_mode_rx(&server->dev, RADIO_RX_TIMEOUT_MS);
	if (0 != rc)
		log_error("unable to switch radio to rx: %d", rc);
}


static void _radio_fetch_rx(server_t * server, bool crc_valid)
{
	int rc;

	rc = sx126x_drv_payload_read(&server->dev, server->rx_buffer, server->rx_buffer_capacity);
	if (0 != rc)
	{
		log_error("unable to read frame from radio buffer: %d", rc);
		return;
	}

	server->rx_crc_valid = crc_valid;
	server->rx_buffer_cookie++;
	if (0 == server->rx_buffer_cookie) server->rx_buffer_cookie = 1;
 	server->rx_buffer_size = server->rx_buffer_capacity;

	log_trace("fetched rx frame from radio");
}

static void _radio_to_router(server_t * server) {

	mavlink_message_t msg = {0};
	mavlink_status_t st = {0};
	uint8_t ret = 0;
	for (int i = 0; i < server->rx_buffer_capacity; i++) {
		ret = mavlink_parse_char(server->mavlink_chan, server->rx_buffer[i], &msg, &st);
	}
	if (ret) {
		log_trace("yay! we've got a message %d", msg.msgid);
		its_rt_sender_ctx_t ctx = {0};
		its_rt_route(&ctx, &msg, SERVER_SMALL_TIMEOUT_MS);
	} else {
		log_warn("what did we've got? %d", server->rx_buffer_capacity);
	}
}

static void _radio_event_handler(sx126x_drv_t * drv, void * user_arg,
		sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg
)
{
	log_trace("Event handler");
	server_t * server = (server_t*)user_arg;

	bool went_tx = false;
	switch (kind)
	{
	case SX126X_EVTKIND_RX_DONE:
		if (arg->rx_done.timed_out)
		{
			log_trace("rx timedout event");
			// Значит мы ничего не получили и эфир свободный
			server->rx_timedout_cnt++;
			// Это странно конечно, мы уже много раз так так сделали?
			if (server->rx_timedout_cnt >= server->rx_timedout_limit)
			{
				if (server->rx_timedout_cnt % server->rx_timedout_limit == 0)
					log_warn("no rx");

				// Мы уже долго ждали, будем отправлять
				//went_tx = _radio_go_tx_if_any(server);
				if (went_tx)
					server->rx_timedout_cnt = 0;
			}
		}
		else
		{
			log_trace("rx done");
			// Мы получили пакет!
			// Выгружаем его с радио
			_radio_fetch_rx(server, arg->rx_done.crc_valid);
			_radio_to_router(server);
			// Тут же пойдем в TX, если нам есть чем
			//went_tx = _radio_go_tx_if_any(server);
		}
		break;

	case SX126X_EVTKIND_TX_DONE:
		if (arg->tx_done.timed_out)
		{
			// Ой, что-то пошло не так
			log_error("TX TIMED OUT!");
			server->tx_cookie_dropped = server->tx_cookie_in_progress;
			server->tx_cookie_in_progress = 0;
		}
		else
		{
			// Все пошло так
			log_info("tx done");
			server->tx_cookie_sent = server->tx_cookie_in_progress;
			server->tx_cookie_in_progress = 0;
		}

		server->tx_cookies_updated = true;
		break;
	default:
		break;
	}

	// Если в результате этого события мы не пошли в TX - уходим в RX
	if (went_tx)
	{
		log_info("tx begun");
		server->rx_timedout_cnt = 0;
	}
	//else
		//_radio_go_rx(server);
}


void _server_sync_rssi(server_t * server)
{
	struct timespec now;
	int rc = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(0 == rc);

	if (sx126x_drv_state(&server->dev) != SX126X_DRVSTATE_RX)
		return;

	const int64_t time_since_last_report = _timespec_diff_ms(&now, &server->rssi_report_block_deadline);
	if (time_since_last_report < server->rssi_report_period)
		return;

	// Пока отправлять rssi
	int8_t rssi;
	rc = sx126x_drv_rssi_inst(&server->dev, &rssi);
	if (0 != rc)
		log_error("unable to get rssi %d", rc);

	server->rssi_report_block_deadline = now;
}


void _server_sync_tx_state(server_t * server)
{
	struct timespec now;
	int rc = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(0 == rc);

	const int64_t time_since_last_report = _timespec_diff_ms(&now, &server->tx_state_report_block_deadline);
	if (!server->tx_cookies_updated && time_since_last_report < server->tx_state_report_period_ms)
		return;

	server->tx_state_report_block_deadline = now;
}


void _server_sync_rx_data(server_t * server)
{
	if (!server->rx_buffer_size)
		return;

}


int server_init(server_t * server)
{
	int rc;
	memset(server, 0x00, sizeof(*server));

	server->rx_timedout_limit = RADIO_RX_TIMEDOUT_LIMIT;
	server->rx_buffer_capacity = RADIO_PACKET_SIZE;
	server->rx_timeout_ms = RADIO_RX_TIMEOUT_MS;

	server->rssi_report_period = RADIO_RSSI_PERIOD_MS;

	server->tx_buffer_capacity = RADIO_PACKET_SIZE;
	server->tx_timeout_ms = RADIO_TX_TIMEOUT_MS;

	server->tx_state_report_period_ms = SERVER_TX_STATE_PERIOD_MS;
	server->mavlink_chan = mavlink_claim_channel();

	rc = _radio_init(server);
	if (rc < 0)
		return rc;

	return 0;
}

void server_start(server_t *server) {
	// Запускам цикл радио
	_radio_go_rx(server);
}

void server_run(server_t * server)
{
	//server_start(server);
	int rc = 0;
	int step = 0;

	while (1) {
		rc = sx126x_drv_poll(&server->dev);
		if (rc) {
			ESP_LOGE("radio", "poll %d", rc);
		}
		if (step % 50 == 0) {
			ESP_LOGD("radio", "Write");
			uint8_t str[] = "Hello World!";
			rc = sx126x_drv_payload_write(&server->dev, str, sizeof(str));
			if (rc) {
				ESP_LOGE("radio", "write %d", rc);
			}

			rc = sx126x_drv_mode_tx(&server->dev, 4000);
			if (rc) {
				ESP_LOGE("radio", "mode_tx %d", rc);
			}
		}

		step++;
		vTaskDelay(100 / portTICK_PERIOD_MS);
		//_server_sync_tx_state(server);
		//_server_sync_rx_data(server);
		//_server_sync_rssi(server);
	}
}

void server_task(void *arg) {
	server_run(arg);
}


void server_deinit(server_t * server)
{
	_radio_deinit(server);
}

