#include "server.h"

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zmq.h>
#include <log.h>
#include <jsmin.h>

#include <sx126x_board_rpi.h>


#define ITS_PUB_ENDPOINT_KEY "ITS_GBUS_BSCP_ENDPOINT"
#define ITS_SUB_ENDPOINT_KEY "ITS_GBUS_BPCS_ENDPOINT"

#define ITS_GBUS_TOPIC_UPLINK_FRAME "radio.uplink_frame"
#define ITS_GBUS_TOPIC_DOWNLINK_FRAME "radio.downlink_frame"
#define ITS_GBUS_TOPIC_UPLINK_STATE "radio.uplink_state"
#define ITS_GBUS_TOPIC_RSSI_INSTANT "radio.rssi_instant"
#define ITS_GBUS_TOPIC_RSSI_PACKET "radio.rssi_packet"



static int64_t _timespec_diff_ms(const struct timespec * left, const struct timespec * right)
{
	return (left->tv_sec - right->tv_sec) * 1000 - (left->tv_nsec - right->tv_nsec) / (1000 * 1000);
}


static int _parse_tx_frame_metadata(
		const char * json_buffer, size_t buffer_size, msg_cookie_t * msg_cookie
)
{
	jsmn_parser parser;
	jsmn_init(&parser);

	jsmntok_t t[3];
	int parsed_tokens = jsmn_parse(&parser, json_buffer, buffer_size, t, sizeof(t)/sizeof(*t));
	if (parsed_tokens != 3)
	{
		log_error(
				"invalid tx meta json \"%s\", expected json in form \"{ cookie: <number> }\": %d",
				parsed_tokens
		);
		return -1;
	}

	if (t[0].type != JSMN_OBJECT || t[1].type != JSMN_STRING || t[2].type != JSMN_PRIMITIVE)
	{
		log_error(
				"invalid tx meta json token types: %d, %d, %d",
				(int)t[0].type, (int)t[1].type, (int)t[2].type
		);
		return -1;
	}

	const jsmntok_t * key_tok = &t[1];
	const char expected_key[] = "cookie";
	const size_t expected_key_size = sizeof(expected_key) - 1;
	if (expected_key_size != key_tok->end - key_tok->start)
	{
		log_error("tx meta json: cookie tooken len mismatch");
		return -1;
	}

	if (0 != strncmp(json_buffer + key_tok->start, expected_key, expected_key_size))
	{
		log_error("invalid tx metadata cookie key");
		return -1;
	}


	const jsmntok_t * value_tok = &t[2];
	const char * const value_str_begin_ptr = json_buffer + value_tok->start;
	char * value_str_end_ptr;

	msg_cookie_t value = strtoull(value_str_begin_ptr, &value_str_end_ptr, 0);
	if (value_str_end_ptr != json_buffer + value_tok->end)
	{
		log_error("unable to parse tx message cookie from json metadata");
		return -1;
	}

	*msg_cookie = value;
	return 0;
}


static void _zmq_deinit(server_t * server)
{
	if (server->sub_socket)
	{
		zmq_close(server->sub_socket);
		server->sub_socket = NULL;
	}

	if (server->pub_socket)
	{
		zmq_close(server->pub_socket);
		server->pub_socket = NULL;
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

	const char * pub_ep = getenv(ITS_PUB_ENDPOINT_KEY);
	if (!pub_ep)
	{
		log_error("unable to get value of pub endpoint from "
				ITS_PUB_ENDPOINT_KEY
				" envvar: %d: %s",
				errno, strerror(errno)
		);
		return -1;
	}

	const char * sub_ep = getenv(ITS_SUB_ENDPOINT_KEY);
	if (!sub_ep)
	{
		log_error("unable to get value of sub endpoint from "
				ITS_SUB_ENDPOINT_KEY
				" envvar: %d: %s",
				errno, strerror(errno)
		);
		return -1;
	}


	server->zmq = zmq_ctx_new();
	if (!server->zmq)
		goto bad_exit;


	server->sub_socket = zmq_socket(server->zmq, ZMQ_SUB);
	if (!server->sub_socket)
	{
		log_error("unable to allocate server sub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	log_info("connecting sub socket to \"%s\"", sub_ep);
	rc = zmq_connect(server->sub_socket, sub_ep);
	if (rc < 0)
	{
		log_error("unable to connect server sub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	const char topic[] = ITS_GBUS_TOPIC_UPLINK_FRAME;
	rc = zmq_setsockopt(server->sub_socket, ZMQ_SUBSCRIBE, topic, sizeof(topic)-1);
	if (rc < 0)
	{
		log_error("unable to subscribe pub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	server->pub_socket = zmq_socket(server->zmq, ZMQ_PUB);
	if (!server->pub_socket)
	{
		log_error("unable to allocate server pub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	log_info("connecting pub socket to \"%s\"", pub_ep);
	rc = zmq_connect(server->pub_socket, pub_ep);
	if (rc < 0)
	{
		log_error("unable to connect server pub socket: %d, %d: %s", rc, errno, strerror(errno));
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
			"sending tx status. Cookies: "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER,
			server->tx_cookie_wait,
			server->tx_cookie_in_progress,
			server->tx_cookie_sent,
			server->tx_cookie_dropped
	);

	// Формируем пакет
	// Протокол требует от нас писать null там где нули
	// Поэтому... придется сперва перекинуть в текст те числа, которые не null
	const char json_null[5] = "null";
	char cookie_str_buffers[4][21] = {0}; // 2**64-1 имеет длину 20 симоволов
	msg_cookie_t * cookies[] = {
			&server->tx_cookie_wait, &server->tx_cookie_in_progress,
			&server->tx_cookie_sent, &server->tx_cookie_dropped
	};
	for (size_t i = 0; i < sizeof(cookies)/sizeof(*cookies); i++)
	{
		if (0 == *cookies[i])
			strcpy(cookie_str_buffers[i], json_null);
		else
		{
			rc = snprintf(
					cookie_str_buffers[i], sizeof(cookie_str_buffers[i]),
					"%"MSG_COOKIE_T_PLSHOLDER, *cookies[i]
			);
			if (rc < 0 || rc >= sizeof(cookie_str_buffers[i]))
			{
				log_error("sprintf for cookie no %d failed: %d, %d: %s", i, rc, errno, strerror(errno));
				return;
			}
		}
	}

	// Теперь наконец-то sprintf самого json-а
	char json_buffer[1024] = {0}; // Вот так круто берем килобайт
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"cookie_in_wait: %s, "
				"cookie_in_progress: %s, "
				"cookie_sent: %s, "
				"cookie_dropped: %s"
			"}",
			cookie_str_buffers[0],
			cookie_str_buffers[1],
			cookie_str_buffers[2],
			cookie_str_buffers[3]
	);

	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("sprintf for tx_state json failed: %d, %d: %s", rc, errno, strerror(errno));
		return;
	}


	// Шлем топик
	const char topic[] = ITS_GBUS_TOPIC_UPLINK_STATE;
	rc = zmq_send(server->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status topic: %d: %s", errno, strerror(errno));
		return;
	}

	// Шлем данные
	rc = zmq_send(server->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status message: %d: %s", errno, strerror(errno));
		return;
	}

	return;
}


static void _zmq_send_packet_rssi(server_t * server)
{
	int rc;

	// Готовим сообщение
	char json_buffer[1024] = {0};
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"cookie: %"MSG_COOKIE_T_PLSHOLDER", "
				"rssi_pkt: %d, "
				"snr_pkt: %d, "
				"rssi_signal: %d"
			"}",
			server->rx_buffer_cookie,
			(int)server->rx_rssi_pkt,
			(int)server->rx_snr_pkt,
			(int)server->rx_signal_rssi_pkt
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("unable to sprintf packet rssi value: %d, %d: %s", rc, errno, strerror(errno));
		return;
	}


	// Топик
	const char topic[] = ITS_GBUS_TOPIC_RSSI_PACKET;
	rc = zmq_send(server->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data topic: %d: %s", errno, strerror(errno));
		return;
	}

	if (rc < 0)
	{
		log_error("sprintf rx rssi json failed: %d, %d: %s", rc, errno, strerror(errno));
		return;
	}

	rc = zmq_send(server->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx rssi data: %d: %s", errno, strerror(errno));
		return;
	}
}


static void _zmq_send_instant_rssi(server_t * server, int8_t rssi)
{
	int rc;
	log_trace("sending rssi %d", (int)rssi);

	// Готовим сообщение
	char json_buffer[1024] = {0};
	rc = snprintf(json_buffer, sizeof(json_buffer), "{rssi: %d}", (int)rssi);
	if (rc < 0)
	{
		log_error("sprintf rssi failed: %d, %d: %s", rc, errno, strerror(errno));
		return;
	}


	// Топик
	const char topic[] = ITS_GBUS_TOPIC_RSSI_INSTANT;
	rc = zmq_send(server->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi topic: %d: %s", errno, strerror(errno));
		return;
	}

	// Само значение
	rc = zmq_send(server->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi data: %d: %s", errno, strerror(errno));
		return;
	}
}


static void _zmq_send_rx_data(server_t * server)
{
	int rc;

	if (0 == server->rx_buffer_size)
		return;

	log_debug("sending rx data");

	// Готовим метаданные
	char json_buffer[1024] = {0};
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"checksum_valid: %s, "
				"cookie: %"MSG_COOKIE_T_PLSHOLDER""
			"}",
			server->rx_crc_valid ? "true" : "false",
			server->rx_buffer_cookie
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("unable to sprintf rx data meta json: %d, %d: %s", rc, errno, strerror(errno));
		goto end;
	}


	// Шлем топик
	const char topic[] = ITS_GBUS_TOPIC_DOWNLINK_FRAME;
	rc = zmq_send(server->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data topic: %d: %s", errno, strerror(errno));
		goto end;
	}

	// метаданные
	rc = zmq_send(server->pub_socket, json_buffer, strlen(json_buffer), ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx metadata: %d, %d: %s", rc, errno, strerror(errno));
		goto end;
	}

	// Теперь данные самого фрейма
	rc = zmq_send(server->pub_socket, server->rx_buffer, server->rx_buffer_size, ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data: %d: %s", errno, errno);
		goto end;
	}

	//В догонку кидаем rssi пакета
	_zmq_send_packet_rssi(server);

end:
	server->rx_buffer_size = 0;
}


static void _zmq_recv_tx_packet(server_t * server)
{
	int rc;
	enum state_t { STATE_TOPIC, STATE_COOKIE, STATE_FRAME, STATE_FLUSH };
	enum state_t state = STATE_TOPIC;

	msg_cookie_t cookie;
	zmq_msg_t msg;
	while(1)
	{
		zmq_msg_init(&msg);
		rc = zmq_msg_recv(&msg, server->sub_socket, ZMQ_DONTWAIT);
		if (rc < 0)
		{
			log_error("unable to get message from data socket %d: %s", errno, strerror(errno));
			zmq_msg_close(&msg);
			break;
		}

		switch (state)
		{
		case STATE_TOPIC: {
			char topic_buffer[1024] = {0};
			const size_t msg_size = zmq_msg_size(&msg);
			if (msg_size > sizeof(topic_buffer))
			{
				log_error("unable to read input message topic. It is too large");
				state = STATE_FLUSH;
				break;
			}
			const char expected_topic[] = ITS_GBUS_TOPIC_UPLINK_FRAME;
			const size_t expected_topic_size = sizeof(expected_topic) - 1;
			if (expected_topic_size != msg_size)
			{
				// Просто молча уйдем. Не тот топик - это еще не катастрофа
				log_debug("skipping input bus message with invalid topic size");
				state = STATE_FLUSH;
				break;
			}
			memcpy(topic_buffer, zmq_msg_data(&msg), msg_size);
			if (0 != strncmp(topic_buffer, expected_topic, msg_size))
			{
				log_debug("skipping input bus message with invalid topic value");
				state = STATE_FLUSH;
				break;
			}

			// Все ок, работаем дальше
			state = STATE_COOKIE;
		} break;

		case STATE_COOKIE: {
			// Мы сейчас копируем куку сообщения
			char json_buffer[1024] = {0};
			const size_t msg_size = zmq_msg_size(&msg);
			if (msg_size > sizeof(json_buffer))
			{
				log_error("unable to receive tx message metadata. message is too big");
				state = STATE_FLUSH;
				break;
			}

			memcpy(json_buffer, zmq_msg_data(&msg), msg_size);
			rc = _parse_tx_frame_metadata(json_buffer, msg_size, &cookie);
			if (rc < 0)
			{
				// Сообщение об ошибке уже написали
				state = STATE_FLUSH;
				break;
			}

			// Все ок, разобралось. Будем получать фрейм
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
			.spreading_factor = SX126X_LORA_SF_5,
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
			server->rx_timedout_cnt = 0;
			// Мы получили пакет!
			// Выгружаем его с радио
			_radio_fetch_rx(server, arg->rx_done.crc_valid);
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
	{
		log_info("tx begun");
		server->rx_timedout_cnt = 0;
	}
	else
		_radio_go_rx(server);
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

	_zmq_send_instant_rssi(server, rssi);
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

	pollitems[1].socket = server->sub_socket;
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
