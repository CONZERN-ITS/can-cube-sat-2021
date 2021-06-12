#include "server.h"

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zmq.h>
#include <log.h>

#include <sx126x_board_rpi.h>


//! разница между двумя таймштампами
static int64_t _timespec_diff_ms(const struct timespec * left, const struct timespec * right)
{
	return (left->tv_sec - right->tv_sec) * 1000 + (left->tv_nsec - right->tv_nsec) / (1000 * 1000);
}


static void _radio_deinit(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;

	// резетим радио на последок
	// Даже если ничего не получается
	sx126x_drv_reset(radio);
	// Деструктим дескриптор
	sx126x_drv_dtor(radio);
}


static int _radio_init(server_t * server)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;

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
			.pa_power = 14,
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
			.preamble_length = 16,
			.explicit_header = true,
			.payload_length = server->config.radio_packet_size,
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
			.lora_symb_timeout = 0,
	};


	rc = sx126x_drv_ctor(radio, NULL);
	if (0 != rc)
		goto bad_exit;

	uint16_t device_errors = 0;
	sx126x_drv_get_device_errors(radio, &device_errors);
	rc = sx126x_drv_reset(radio);
	log_info("after reset; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;

	device_errors = 0;
	rc = sx126x_drv_configure_basic(radio, &basic_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure basic; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	device_errors = 0;
	sx126x_drv_get_device_errors(radio, &device_errors);
	rc = sx126x_drv_configure_lora_modem(radio, &modem_cfg);
	log_info("configure lora modem; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	device_errors = 0;
	rc = sx126x_drv_configure_lora_packet(radio, &packet_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure lora packet; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	rc = sx126x_drv_configure_lora_cad(radio, &cad_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure lora cad; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	rc = sx126x_drv_configure_lora_rx_timeout(radio, &rx_timeout_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure lora rx timeout; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	device_errors = 0;
	rc = sx126x_drv_mode_standby_rc(radio);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure standby_rc; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;

	device_errors = 0;
	rc = sx126x_drv_mode_standby(radio);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure standby default; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;

	return 0;

bad_exit:
	_radio_deinit(server);
	return -1;
}


static void _load_tx(server_t * server)
{
	int rc;

	size_t packet_size;
	msg_cookie_t packet_cookie;

	rc = zserver_recv_tx_packet(
		&server->zserver,
		server->tx_buffer, server->config.tx_buffer_capacity,
		&packet_size, &packet_cookie
	);

	if (0 != rc)
	{
		log_error("unable to fetch tx packet from zmd: %d", rc);
		return;
	}

	server->tx_buffer_size = packet_size;
	server->tx_cookie_wait = packet_cookie;
	server->tx_cookies_updated = true;

	if (server->tx_buffer_size)
		log_info(
			"loaded tx frame %"MSG_COOKIE_T_PLSHOLDER" with size %zu", \
			server->tx_cookie_wait, server->tx_buffer_size
		);
}


static bool _go_tx(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;

	int rc;
	bool retval = true;

	if (0 == server->tx_cookie_wait || 0 == server->tx_buffer_size)
		return false;

	log_trace("going tx");
	rc = sx126x_drv_payload_write(radio, server->tx_buffer, server->tx_buffer_size);
	if (0 != rc)
	{
		log_error("unable to write tx payload to radio: %d. Dropping frame", rc);
		server->tx_cookie_dropped = server->tx_cookie_wait;
		retval = false;
		goto end;
	}

	rc = sx126x_drv_mode_tx(radio, server->config.tx_timeout_ms);
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


static void _process_tx_done(server_t * server, bool succeed)
{
	// Все очень просто
	if (succeed)
	{
		log_debug("tx done");
		server->tx_cookie_sent = server->tx_cookie_in_progress;
	}
	else
	{
		log_error("TX TIMED OUT");
		server->tx_cookie_dropped = server->tx_cookie_in_progress;
	}

	server->tx_cookie_in_progress = 0;
}


static void _go_rx(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;
	int rc;

	rc = sx126x_drv_mode_rx(radio, server->config.rx_timeout_ms);
	if (0 != rc)
		log_error("unable to switch radio to rx: %d", rc);

	log_trace("rx begun");
}


static void _fetch_rx(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;
	int rc;

	uint8_t payload_size = 0;
	rc = sx126x_drv_payload_rx_size(radio, &payload_size);
	if (0 != rc)
	{
		log_error("unable to read frame size from radio: %d", rc);
		return;
	}

	uint8_t payload[SERVER_MAX_PACKET_SIZE] = { 0x00 };
	rc = sx126x_drv_payload_read(radio, payload, payload_size);
	if (0 != rc)
	{
		log_error("unable to read frame data from radio buffer: %d", rc);
		return;
	}

	bool crc_valid;
	rc = sx126x_drv_payload_rx_crc_valid(radio, &crc_valid);
	if (0 != rc)
	{
		log_error("unable to check rx frame crc value: %d", rc);
		return;
	}

	sx126x_lora_packet_status_t packet_status;
	rc = sx126x_drv_lora_packet_status(radio, &packet_status);
	if (0 != rc)
	{
		log_error("unable to fetch lora packet status: %d", rc);
		return;
	}

	log_trace(
		"fetched rx frame from radio. "
		"crc_valid: %s, packet_rssi: %d, packet_snr: %d, signal_rssi: %d",
		crc_valid ? "true" : "false",
		(int)packet_status.rssi_pkt, (int)packet_status.snr_pkt, (int)packet_status.signal_rssi_pkt
	);

	// Отправляем полученное через zmq
	msg_cookie_t cookie = server->rx_cookie++;
	zserver_send_rx_packet(
			&server->zserver,
			payload, payload_size,
			cookie, crc_valid,
			packet_status.rssi_pkt, packet_status.snr_pkt, packet_status.signal_rssi_pkt
	);

	zserver_send_packet_rssi(&server->zserver,
			cookie,
			packet_status.rssi_pkt, packet_status.snr_pkt, packet_status.signal_rssi_pkt
	);

	// Коды ошибки не проверяем. Черт с ним, мы пыталисьs
}


static void _radio_event_handler(server_t * server, const sx126x_drv_evt_t * event)
{
	sx126x_drv_t * const radio = &server->radio;

	bool went_tx = false;
	switch (event->kind)
	{
	case SX126X_DRV_EVTKIND_RX_DONE:
		if (event->arg.rx_done.timed_out)
		{
			log_trace("rx timedout event %d", (int)server->rx_timedout_cnt);
			// Значит мы ничего не получили и эфир свободный
			server->rx_timedout_cnt++;
			// Это странно конечно, мы уже много раз так так сделали?
			if (server->rx_timedout_cnt >= server->config.rx_timeout_limit)
			{
				if (server->rx_timedout_cnt % server->config.rx_timeout_limit == 0)
					log_warn("no rx");

				// Мы уже долго ждали, будем отправлять
				went_tx = _go_tx(server);
				if (went_tx)
					server->rx_timedout_cnt = 0;
			}
		}
		else
		{
			log_trace("rx done");
			server->rx_timedout_cnt = 0;
			// Мы получили пакет!
			// Выгружаем его с радио
			_fetch_rx(server);
			// Тут же пойдем в TX, если нам есть чем
			went_tx = _go_tx(server);
		}
		break;

	case SX126X_DRV_EVTKIND_TX_DONE:
		_process_tx_done(server, !event->arg.tx_done.timed_out);
		break;
	}

	// Если в результате этого события мы не пошли в TX - уходим в RX
	if (went_tx)
	{
		log_info("tx begun");
		server->rx_timedout_cnt = 0;
	}
	else
		_go_rx(server);
}


static void _report_rssi(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;
	int rc;

	if (sx126x_drv_state(radio) != SX126X_DRV_STATE_RX)
		return;

	int8_t rssi;
	rc = sx126x_drv_rssi_inst(radio, &rssi);
	if (0 != rc)
		log_error("unable to get rssi %d", rc);

	zserver_send_instant_rssi(&server->zserver, rssi);
}


static void _report_tx_state(server_t * server)
{
	zserver_send_tx_buffers_state(
			&server->zserver,
			server->tx_cookie_wait, server->tx_cookie_in_progress,
			server->tx_cookie_sent, server->tx_cookie_dropped
	);
	// Ошибки не проверяем. Мы пытались
	server->tx_cookies_updated = false;
}


static void _report_radio_stats(server_t * server)
{
	sx126x_drv_t * const radio = &server->radio;
	int rc;

	uint16_t device_errors;
	rc = sx126x_drv_get_device_errors(radio, &device_errors);
	if (0 != rc)
	{
		log_error("unable to get device errors: %d", rc);
		return;
	}

	rc = sx126x_drv_clear_device_errors(radio);
	if (0 != rc)
	{
		log_error("unable to clear device errors");
		return;
	}

	// К сожалению у нас на практике дофига ошибок SX126X_DEVICE_ERROR_XOSC_START
	// Они проявляются на каждом модуле и никак не удается их снять
	// Поэтому мы просто будем их игнорировать
	const uint16_t masked_errors = device_errors & ~server->config.ignored_errors;
	if (masked_errors)
	{
		char errors_str[1024] = {0};
		if (masked_errors & SX126X_DEVICE_ERROR_RC64K_CALIB)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_RC64K_CALIB");

		if (masked_errors & SX126X_DEVICE_ERROR_RC13M_CALIB)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_RC13M_CALIB");

		if (masked_errors & SX126X_DEVICE_ERROR_PLL_CALIB)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_PLL_CALIB");

		if (masked_errors & SX126X_DEVICE_ERROR_ADC_CALIB)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_ADC_CALIB");

		if (masked_errors & SX126X_DEVICE_ERROR_IMG_CALIB)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_IMG_CALIB");

		if (masked_errors & SX126X_DEVICE_ERROR_XOSC_START)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_XOSC_START");

		if (masked_errors & SX126X_DEVICE_ERROR_PLL_LOCK)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_PLL_LOCK");

		if (masked_errors & SX126X_DEVICE_ERROR_PA_RAMP)
			strcat(errors_str, ", SX126X_DEVICE_ERROR_PA_RAMP");

		const char * errors_str_begin = errors_str + 2; // Сдвигаем 2 символа на первые ", "
		log_error("detected device errors: 0x%04"PRIx16": %s", device_errors, errors_str_begin);

		uint16_t known_bits = SX126X_DEVICE_ERROR_RC64K_CALIB
			| SX126X_DEVICE_ERROR_RC13M_CALIB
			| SX126X_DEVICE_ERROR_PLL_CALIB
			| SX126X_DEVICE_ERROR_ADC_CALIB
			| SX126X_DEVICE_ERROR_IMG_CALIB
			| SX126X_DEVICE_ERROR_XOSC_START
			| SX126X_DEVICE_ERROR_PLL_LOCK
			| SX126X_DEVICE_ERROR_PA_RAMP
		;

		uint16_t unknown_bits = device_errors & ~known_bits;
		if (unknown_bits)
			log_error("detected unknown error bits: 0x%04"PRIx16"", unknown_bits);
	}

	// Выдаем сообщение
	sx126x_stats_t stats;
	rc = sx126x_drv_get_stats(radio, &stats);
	if (0 != rc)
	{
		log_error("unable to get radio stats: %d", rc);
		return;
	}

	zserver_send_radio_stats(&server->zserver, &stats, device_errors);
}


static void _do_periodic_jobs(server_t * server)
{
	int rc;

	struct timespec now;
	rc = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(0 == rc);

	int64_t time_since_last_report;

	time_since_last_report = _timespec_diff_ms(&now, &server->rssi_last_report_timepoint);
	if (time_since_last_report >= server->config.rssi_report_period_ms)
	{
		_report_rssi(server);
		server->rssi_last_report_timepoint = now;
	}

	time_since_last_report = _timespec_diff_ms(&now, &server->tx_state_last_report_timepoint);
	if (server->tx_cookies_updated || time_since_last_report >= server->config.tx_state_report_period_ms)
	{
		_report_tx_state(server);
		server->tx_state_last_report_timepoint = now;
	}

	time_since_last_report = _timespec_diff_ms(&now, &server->radio_stats_last_report_timepoint);
	if (time_since_last_report >= server->config.radio_stats_report_period_ms)
	{
		_report_radio_stats(server);
		server->radio_stats_last_report_timepoint = now;
	}
}


int server_init(server_t * server, const server_config_t * config)
{
	int rc;
	memset(server, 0x00, sizeof(*server));
	server->config = *config;

	rc = zserver_init(&server->zserver);
	if (rc != 0)
		return rc;

	rc = _radio_init(server);
	if (rc != 0)
		return rc;

	return 0;
}


void server_run(server_t * server)
{
	// Запускам цикл радио
	_go_rx(server);

	sx126x_drv_t * const radio = &server->radio;
	int radio_irq_fd = sx126x_brd_rpi_get_event_fd(radio->api.board);

	zmq_pollitem_t pollitems[2] = {0};
	pollitems[0].fd = radio_irq_fd;
	pollitems[0].events = ZMQ_POLLIN | ZMQ_POLLPRI;

	pollitems[1].socket = server->zserver.sub_socket;
	pollitems[1].events = ZMQ_POLLIN;

	while (1)
	{
		int rc = zmq_poll(pollitems, sizeof(pollitems)/sizeof(*pollitems), server->config.poll_timeout_ms);
		if (rc < 0)
		{
			perror("zmq poll error");
			break;
		}

		if (pollitems[0].revents)
		{
			// Ага, что-то произошло с радио
			log_trace("radio event: %d", pollitems[0].revents);
			rc = sx126x_brd_rpi_cleanup_event(radio->api.board);
			if (0 != rc)
				log_error("unable to clean up radio IRQ: %d", rc);
		}

		if (pollitems[1].revents)
		{
			log_trace("zmq_event %d", pollitems[1].revents);

			// Что-то пришло, что хотят отправить на верх
			_load_tx(server);
		}

		sx126x_drv_evt_t radio_event;
		rc = sx126x_drv_poll_event(radio, &radio_event);
		if (0 == rc)
			_radio_event_handler(server, &radio_event);
		else
			log_error("radio poll error %d", rc);

		_do_periodic_jobs(server);
	} // while (1)
}


void server_deinit(server_t * server)
{
	zserver_deinit(&server->zserver);
	_radio_deinit(server);
}
