#include "server.h"

#include <poll.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zmq.h>
#include <log.h>

#include <sx126x_board_rpi.h>


// Удобный шорткат для получения текущей метки времениs
static struct timespec _timespec_now(void)
{
	struct timespec rv;
	int rc = clock_gettime(CLOCK_MONOTONIC, &rv);
	assert(0 == rc);
	return rv;
}


//! разница между двумя таймштампами
static int64_t _timespec_diff_ms(const struct timespec * left, const struct timespec * right)
{
	return (left->tv_sec - right->tv_sec) * 1000 + (left->tv_nsec - right->tv_nsec) / (1000 * 1000);
}


//! Сколько времени прошло от then до сейчас
static int64_t _timespec_diff_ms_now(const struct timespec * then)
{
	struct timespec now = _timespec_now();
	return _timespec_diff_ms(&now, then);
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
	rc = sx126x_drv_configure_basic(radio, &server->config.radio_basic_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure basic; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	device_errors = 0;
	sx126x_drv_get_device_errors(radio, &device_errors);
	rc = sx126x_drv_configure_lora_modem(radio, &server->config.radio_modem_cfg);
	log_info("configure lora modem; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	device_errors = 0;
	rc = sx126x_drv_configure_lora_packet(radio, &server->config.radio_packet_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure lora packet; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	rc = sx126x_drv_configure_lora_cad(radio, &server->config.radio_cad_cfg);
	sx126x_drv_get_device_errors(radio, &device_errors);
	log_info("configure lora cad; rc = %d, device_errors: 0x%04"PRIx16"", rc, device_errors);
	if (0 != rc)
		goto bad_exit;


	rc = sx126x_drv_configure_lora_rx_timeout(radio, &server->config.radio_rx_timeout_cfg);
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

	memset(server->tx_buffer, 0x00, server->config.radio_packet_cfg.payload_length);
	rc = zserver_recv_tx_packet(
		&server->zserver,
		server->tx_buffer, server->config.radio_packet_cfg.payload_length,
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

	// Будем писать об ошибках только когда они появляются
	const uint16_t masked_errors = device_errors & ~server->radio_errors;
	server->radio_errors = device_errors;
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

	// Проверяем не прислали ли нам чего
	zmq_pollitem_t pollitems[1] = {
			{
					.socket = server->zserver.sub_socket,
					.events = ZMQ_POLLIN
			}
	};

	rc = zmq_poll(pollitems, 1, 0);
	if (rc >= 0)
	{
		if (pollitems[0].revents & ZMQ_POLLIN)
			_load_tx(server);
	}
	else
	{
		log_error("network poll failed! %d", rc);
	}

	// Дальше занимаемся сбором всякой статитистики и отправкой её на шину сообщений
	int64_t time_since_last_report;

	time_since_last_report = _timespec_diff_ms_now(&server->rssi_last_report_timepoint);
	if (time_since_last_report >= server->config.rssi_report_period_ms)
	{
		_report_rssi(server);
		server->rssi_last_report_timepoint = _timespec_now();
	}

	time_since_last_report = _timespec_diff_ms_now(&server->tx_state_last_report_timepoint);
	if (server->tx_cookies_updated || time_since_last_report >= server->config.tx_state_report_period_ms)
	{
		_report_tx_state(server);
		server->tx_state_last_report_timepoint = _timespec_now();
	}

	time_since_last_report = _timespec_diff_ms_now(&server->radio_stats_last_report_timepoint);
	if (time_since_last_report >= server->config.radio_stats_report_period_ms)
	{
		_report_radio_stats(server);
		server->radio_stats_last_report_timepoint = _timespec_now();
	}
}


static int _sleep_poll_radio_interrupt(server_t * server, uint32_t timeout_ms)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;
	int radio_interrupt_fd = sx126x_brd_rpi_get_event_fd(radio->api.board);

	struct pollfd pfd = { .fd = radio_interrupt_fd, .events = POLLIN};
	rc = poll(&pfd, 1, timeout_ms);
	if (rc < 0)
	{
		log_fatal("radio poll failed. rc: %d, errno: %d", rc, errno);
		return rc;
	}

	if (pfd.revents & POLLIN)
	{
		// Интеррупт был, нужно его почистить
		sx126x_brd_rpi_cleanup_event(radio->api.board);
	}

	return 0;
}


static int _go_rx(server_t * server)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;
	const uint32_t hw_timeout = server->config.rx_timeout_ms;

	// Уходим в RX
	rc = sx126x_drv_mode_rx(radio, hw_timeout);
	if (0 != rc)
	{
		log_error("unable to switch radio to rx mode: %d", rc);
		return rc;
	}

	return 0;
}


static int _wait_for_rx(server_t * server, bool * got_packet)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;

	const int poll_timeout = server->config.poll_timeout_ms;
	const uint32_t watchdog_limit = server->config.rx_watchdog_ms;
	struct timespec wait_start;
	wait_start = _timespec_now();

	while(1)
	{
		// Работаем над фоновыми задачами
		_do_periodic_jobs(server);

		// Чуток поспим с условим проснуться по прерыванию
		rc = _sleep_poll_radio_interrupt(server, poll_timeout);
		if (0 != rc)
			return rc;

		// Проверяем события драйвера
		sx126x_drv_evt_t event;
		rc = sx126x_drv_poll_event(radio, &event);
		if (0 != rc)
		{
			log_error("unable to poll radio event (in rx): %d", rc);
			return rc;
		}

		if (SX126X_DRV_EVTKIND_RX_DONE == event.kind)
		{
			if (event.arg.rx_done.timed_out)
			{
				*got_packet = false;
			}
			else
			{
				// Есть пакет! Выграебаем!
				_fetch_rx(server);
				*got_packet = true;
			}

			// Вне зависимости - был ли таймаут, отсюда выходим
			break;
		}
		else if (SX126X_DRV_EVTKIND_NONE == event.kind)
		{
			// Ничего не произошло в радио
			// штош, ничего и не делаем, пойдем дальше по телу цикла
		}
		else
		{
			// Произошла какая-то неожиданная петрушка, мы таких событий не ждали
			log_error("unexpected event kind in rx cycle: %d", rc);
			return -1;
		}

		// Не слишком ли мы долго работаем?
		if (_timespec_diff_ms_now(&wait_start) > watchdog_limit)
		{
			// Слишком!
			log_error("RX CYCLE WATCHDOG FIRED");
			return -ETIMEDOUT;
		}
	} // while

	return 0;
}


static int _go_tx(server_t * server)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;
	const uint32_t hw_timeout = server->config.tx_timeout_ms;

	rc = sx126x_drv_payload_write(radio, server->tx_buffer, server->config.radio_packet_cfg.payload_length);
	if (0 != rc)
	{
		log_error("unable to write tx payload to radio: %d", rc);
		return rc;
	}

	rc = sx126x_drv_mode_tx(radio, hw_timeout);
	if (0 != rc)
	{
		log_error("unable to switch radio to tx mode: %d", rc);
		return rc;
	}

	return 0;
}


static int _wait_for_tx(server_t * server, bool * succeed)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;
	int radio_interrput_fd = sx126x_brd_rpi_get_event_fd(radio->api.board);

	const int poll_timeout = server->config.poll_timeout_ms;
	const uint32_t watchdog_limit = server->config.tx_watchdog_ms;
	struct timespec wait_start;
	wait_start = _timespec_now();

	while(1)
	{
		// Работаем над фоновыми задачами
		_do_periodic_jobs(server);

		// Чуток поспим с условим проснуться по прерыванию
		rc = _sleep_poll_radio_interrupt(server, poll_timeout);
		if (0 != rc)
			return rc;

		// Проверяем события драйвера
		sx126x_drv_evt_t event;
		rc = sx126x_drv_poll_event(radio, &event);
		if (0 != rc)
		{
			log_error("unable to poll radio event (in tx): %d", rc);
			return rc;
		}

		if (SX126X_DRV_EVTKIND_TX_DONE == event.kind)
		{
			if (event.arg.tx_done.timed_out)
			{
				log_error("TX TIMED OUT!!!11");
				*succeed = false;
			}
			else
			{
				*succeed = true;
			}

			// Вне зависимости - был ли таймаут, отсюда выходим
			break;
		}
		else if (SX126X_DRV_EVTKIND_NONE == event.kind)
		{
			// Ничего не произошло в радио
			// штош, ничего и не делаем, пойдем дальше по телу цикла
		}
		else
		{
			// Произошла какая-то неожиданная петрушка, мы таких событий не ждали
			log_error("unexpected event kind in tx cycle: %d", rc);
			return -1;
		}

		// Не слишком ли мы долго работаем?
		if (_timespec_diff_ms_now(&wait_start) > watchdog_limit)
		{
			// Слишком!
			log_error("TX CYCLE WATCHDOG FIRED");
			return -ETIMEDOUT;
		}
	} // while

	return 0;
}


static int _server_loop(server_t * server)
{
	int rc;
	sx126x_drv_t * const radio = &server->radio;
	const size_t rx_timeout_limit = server->config.rx_timeout_limit;

begin_rx:
	do
	{
		log_trace("going rx");

		rc = _go_rx(server);
		if (0 != rc)
			return rc;

		log_trace("rx begun");
		// Дальше мы собственно ждем пока этот приём закончится
		bool got_packet;
		rc = _wait_for_rx(server, &got_packet);
		if (0 != rc)
			return rc;

		log_trace("rx operation complete");
		if (got_packet)
		{
			server->rx_packet_received = _timespec_now();
			server->rx_timeout_count = 0;
			log_trace("rx yielded packet");
		}
		else
		{
			server->rx_timeout_count++;
			log_trace("rx timed out");
		}

		// Крутимся тут пока не убедимся что эфир свободен
	} while(server->rx_timeout_count < rx_timeout_limit);


	// Раз мы оказались тут, то это означает что эфир свободен
	log_warn("no rx, channel free. Ready for TX");


	// А есть чего передавать то?
	if (0 == server->tx_cookie_wait)
		// Нет, нечего, вовзращаемся к приёму
		goto begin_rx;

	// Видимо есть, поехали
	log_trace("going tx");

	rc = _go_tx(server);
	if (0 != rc)
	{
		server->tx_cookie_dropped = server->tx_cookie_wait;
		server->tx_cookie_wait = 0;
		server->tx_cookies_updated = true;
		return rc;
	}

	server->tx_cookie_in_progress = server->tx_cookie_wait;
	server->tx_cookie_wait = 0;
	server->tx_cookies_updated = true;
	log_info("tx begun");

	// Ждем завершения
	bool tx_succeed;
	rc = _wait_for_tx(server, &tx_succeed);
	if (0 != rc)
	{
		log_error("TX FAILED SPECTACULAR: %d\n");
		server->tx_cookie_dropped = server->tx_cookie_in_progress;
		server->tx_cookie_in_progress = 0;
		server->tx_cookies_updated = true;
		return rc;
	}

	if (tx_succeed)
	{
		log_info("tx completed");
		server->tx_cookie_sent = server->tx_cookie_in_progress;
		server->tx_cookie_in_progress = 0;
		server->tx_cookies_updated = true;
	}
	else
	{
		log_error("tx failed, but not so spectacular (hw timeout?)");
		server->tx_cookie_dropped = server->tx_cookie_in_progress;
		server->tx_cookie_in_progress = 0;
		server->tx_cookies_updated = true;
		// Завершаться тут не будем. так как с радио все вроде как ок так-то
	}

	// Начинаем все с начала
	goto begin_rx;
}


int server_task(server_t * server, const server_config_t * config)
{
	int rc;
	memset(server, 0x00, sizeof(*server));
	server->config = *config;

	rc = zserver_init(&server->zserver);
	if (rc != 0)
		return rc;

again:
	rc = _radio_init(server);
	if (rc != 0)
	{
		log_error("unable to initialize radio: %d. Will retry", rc);
		sleep(1);
		goto again;
	}
	log_info("radio intialized");

	// Крутимся!
	_server_loop(server);

	log_info("server loop terminated");

	zserver_deinit(&server->zserver);
	_radio_deinit(server);
	return 0;
}


