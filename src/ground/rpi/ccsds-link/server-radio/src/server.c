#include "server.h"

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <zmq.h>
#include <log.h>

#include <sx126x_board_rpi.h>


static int64_t _timespec_diff_ms(const struct timespec * left, const struct timespec * right)
{
	return (left->tv_sec - right->tv_sec) * 1000 - (left->tv_nsec - right->tv_nsec) / (1000 * 1000);
}


static void _zmq_deinit(server_t * server)
{
	if (server->uplink_socket)
	{
		zmq_close(server->uplink_socket);
		server->uplink_socket = NULL;
	}

	if (server->telemetry_socket)
	{
		zmq_close(server->telemetry_socket);
		server->telemetry_socket = NULL;
	}

	if (server->zmq)
	{
		int rc = zmq_ctx_destroy(server->zmq);
		if (rc < 0)
		{
			switch (errno)
			{
			case EFAULT:
				break; // Это был не контекст, черт с ним

			case EINTR:
				// Нас остановили сигналом
				// Ну и черт с ним, завершаемся так
				break;
			}; // switch
		} // if rc

		server->zmq = NULL;
	}
}


static int _zmq_init(server_t * server)
{
	int rc;


	server->zmq = zmq_ctx_new();
	if (!server->zmq)
		goto bad_exit;


	server->uplink_socket = zmq_socket(server->zmq, ZMQ_SUB);
	if (!server->uplink_socket)
	{
		log_error("unable to allocate server data socket: %d", errno);
		goto bad_exit;
	}

	rc = zmq_bind(server->uplink_socket, SERVER_UPLINK_SOCKET_EP);
	if (rc < 0)
	{
		log_error("unable to bind server data socket: %d", errno);
		goto bad_exit;
	}

	const char topic[] = "radio.uplink_data";
	rc = zmq_setsockopt(server->uplink_socket, ZMQ_SUBSCRIBE, topic, sizeof(topic)-1);
	if (rc < 0)
	{
		log_error("unable to subscribe uplink socket: %d", errno);
		goto bad_exit;
	}

	server->telemetry_socket = zmq_socket(server->zmq, ZMQ_PUB);
	if (!server->telemetry_socket)
	{
		log_error("unable to allocate server telemetry socket: %d", errno);
		goto bad_exit;
	}

	rc = zmq_bind(server->telemetry_socket, SERVER_TELEMETRY_SOCKET_EP);
	if (rc < 0)
	{
		log_error("unable to bind server telemetry socket: %d", errno);
		goto bad_exit;
	}

	return 0;

bad_exit:
	_zmq_deinit(server);
	return -1;
}


static void _zmq_send_tx_state(server_t * server)
{
	int rc;

	// Отпрвляем куки TX фреймов, чтобы показать хосту как они раскиданы у нас в буферах
	log_debug(
			"sending tx status. Cookies: %d %d %d %d",
			server->tx_cookie_wait,
			server->tx_cookie_in_progress,
			server->tx_cookie_sent,
			server->tx_cookie_dropped
	);

	// Формируем пакет
	char json_buffer[1024] = {0}; // Ну килобайта то нам хватит ведь правда
	rc = sprintf(json_buffer, "{ wait: %d, in_progress: %d, sent: %d, dropped: %d }",
			server->tx_cookie_wait,
			server->tx_cookie_in_progress,
			server->tx_cookie_sent,
			server->tx_cookie_dropped
	);

	if (rc < 0)
	{
		log_error("sprintf tx status json failed: %d, %d", rc, errno);
		return;
	}

	// Шлем топик
	const char topic[] = "radio.upink_status";
	rc = zmq_send(server->telemetry_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status topic: %d", errno);
		return;
	}

	// Шлем данные
	rc = zmq_send(server->telemetry_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status message: %d", errno);
		return;
	}

	return;
}


static void _zmq_send_rx_data(server_t * server)
{
	int rc;

	if (0 == server->rx_buffer_size)
		return;

	log_info("sending rx data");

	// Шлем топик
	const char topic[] = "radio.downlink_data";
	rc = zmq_send(server->telemetry_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data topic: %d", errno);
		goto end;
	}

	// Теперь сами данные
	rc = zmq_send(server->telemetry_socket, server->rx_buffer, server->rx_buffer_size, ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data: %d", errno);
		goto end;
	}

	//В догонку кидаем rssi пакета
	_zmq_send_packet_rssi(server);

end:
	server->rx_buffer_size = 0;
}


static void _zmq_send_packet_rssi(server_t * server)
{
	int rc;

	char json_buffer[1024] = {0};
	rc = sprintf(json_buffer, "{ rssi_pkt: %d, snr_pkt: %d, rssi_signal: %d }",
			(int)server->rx_rssi_pkt,
			(int)server->rx_snr_pkt,
			(int)server->rx_signal_rssi_pkt
	);

	// Топик
	const char topic[] = "radio.downlink_data";
	rc = zmq_send(server->telemetry_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data topic: %d", errno);
		goto end;
	}

	if (rc < 0)
	{
		log_error("sprintf rx rssi json failed: %d, %d", rc, errno);
		goto end;
	}

	rc = zmq_send(server->telemetry_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx rssi data: %d", errno);
		goto end;
	}

end:
	return;
}


static void _zmq_send_instant_rssi(server_t * server, int8_t rssi)
{
	int rc;
	log_trace("sending rssi %d", (int)rssi);

	// Топик
	const char topic[] = "radio.instant_rssi";
	rc = zmq_send(server->telemetry_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi topic: %d", errno);
		return;
	}

	// Само значение
	char json_buffer[1024] = {0};
	rc = sprintf(json_buffer, "{ rssi: %d }", rssi);
	if (rc < 0)
	{
		log_error("sprintf rssi failed: %d, %d", rc, errno);
		return;
	}

	rc = zmq_send(server->telemetry_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi data: %d", errno);
		return;
	}
}



static void _zmq_recv_tx_packet(server_t * server)
{
	int rc;
	enum state_t { STATE_COOKIE, STATE_FRAME, STATE_FLUSH };
	enum state_t state = STATE_COOKIE;

	uint16_t cookie;
	zmq_msg_t msg;
	while(1)
	{
		zmq_msg_init(&msg);
		rc = zmq_msg_recv(&msg, server->uplink_socket, ZMQ_DONTWAIT);
		if (rc < 0)
		{
			log_error("unable to get message from data socket %d", errno);
			zmq_msg_close(&msg);
			break;
		}

		switch (state)
		{
		case STATE_COOKIE: {
			// Мы сейчас копируем куку сообщения
			const size_t msg_size = zmq_msg_size(&msg);
			if (sizeof(cookie) > msg_size)
			{
				log_error("unable to load tx frame cookie. Message is too small (%d)", msg_size);
				state = STATE_FLUSH;
				break;
			}

			memcpy(&cookie, zmq_msg_data(&msg), sizeof(cookie));
			state = STATE_FRAME;
			} break;

		case STATE_FRAME: {
			// Мы сейчас копируем само сообщение
			size_t frame_size = zmq_msg_size(&msg);
			if (0 == frame_size)
			{
				log_error("unable to load tx frame. Message have zero size");
				break;
			}

			if (frame_size > server->tx_buffer_capacity)
				frame_size = server->tx_buffer_capacity;

			memset(server->tx_buffer, 0x00, server->tx_buffer_capacity);
			memcpy(server->tx_buffer, zmq_msg_data(&msg), frame_size);
			server->tx_buffer_size = frame_size;
			server->tx_cookie_wait = cookie;
			server->tx_cookies_updated = true;
			state = STATE_FLUSH;
			} break;

		default:
			// Мы либо уже готовы, либо сдались
			// флушим остатки MORE собщения, которые у него могут быть
			// Короче ничего не делаем
			break;
		}

		zmq_msg_close(&msg);
		if (!zmq_msg_more(&msg))
			break;
	} // while
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


static void _radio_fetch_rx(server_t * server)
{
	int rc;

	rc = sx126x_drv_payload_read(&server->dev, server->rx_buffer, server->rx_buffer_capacity);
	if (0 != rc)
	{
		log_error("unable to read frame from radio buffer: %d", rc);
		return;
	}

	server->tx_buffer_size = server->rx_buffer_capacity;
	log_trace("fetched rx frame from radio");
}


static void _radio_event_handler(sx126x_drv_t * drv, void * user_arg,
		sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg
)
{
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
				went_tx = _radio_go_tx_if_any(server);
				if (went_tx)
					server->rx_timedout_cnt = 0;
			}
		}
		else
		{
			log_trace("rx done");
			// Мы получили пакет!
			// Выгружаем его с радио
			_radio_fetch_rx(server);
			// Тут же пойдем в TX, если нам есть чем
			went_tx = _radio_go_tx_if_any(server);
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
	}

	// Если в результате этого события мы не пошли в TX - уходим в RX
	if (went_tx)
		log_info("tx begun");
	else
		_radio_go_rx(server);
}


void _server_sync_rssi(server_t * server)
{
	struct timespec now;
	int rc = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(rc);

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

	_zmq_send_instant_rssi(server, rssi);
	server->rssi_report_block_deadline = now;
}


void _server_sync_tx_state(server_t * server)
{
	struct timespec now;
	int rc = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(rc);

	const int64_t time_since_last_report = _timespec_diff_ms(&now, &server->tx_state_report_block_deadline);
	if (!server->tx_cookies_updated && time_since_last_report < server->tx_state_report_period_ms)
		return;

	_zmq_send_tx_state(server);
	server->tx_state_report_block_deadline = now;
}


void _server_sync_rx_data(server_t * server)
{
	if (!server->rx_buffer_size)
		return;

	_zmq_send_rx_data(server);
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

	rc = _zmq_init(server);
	if (rc < 0)
		return rc;

	rc = _radio_init(server);
	if (rc < 0)
		return rc;

	return 0;
}


void server_run(server_t * server)
{
	// Запускам цикл радио
	_radio_go_rx(server);

	int radio_fd = sx126x_brd_rpi_get_event_fd(server->dev.api.board);

	zmq_pollitem_t pollitems[2] = {0};
	pollitems[0].fd = radio_fd;
	pollitems[0].events = ZMQ_POLLIN | ZMQ_POLLPRI;

	pollitems[1].socket = server->uplink_socket;
	pollitems[1].events = ZMQ_POLLIN;

	while (1)
	{
		int rc = zmq_poll(pollitems, sizeof(pollitems)/sizeof(*pollitems), SERVER_POLL_TIMEOUT_MS);
		if (rc < 0)
		{
			perror("zmq poll error");
			break;
		}

		if (pollitems[0].revents)
		{
			log_trace("radio event: %d", pollitems[0].revents);

			// Ага, что-то прозошло с радио
			rc = sx126x_drv_poll(&server->dev);
			if (0 != rc)
				log_error("radio poll error %d", rc);
		}

		if (pollitems[1].revents)
		{
			log_trace("zmq_event %d", pollitems[1].revents);

			// Что-то пришло, что хотят отправить на верх
			_zmq_recv_tx_packet(server);
			if (server->tx_buffer_size)
				log_info("loaded tx frame %d with size %zd", server->tx_cookie_wait, server->tx_buffer_size);
		}

		_server_sync_tx_state(server);
		_server_sync_rx_data(server);
		_server_sync_rssi(server);

	} // while (1)
}


void server_deinit(server_t * server)
{
	_zmq_deinit(server);
	_radio_deinit(server);
}
