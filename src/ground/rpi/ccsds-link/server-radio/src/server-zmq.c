#include "server-zmq.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <zmq.h>
#include <log.h>
#include <jsmin.h>


#include "server.h"


#define ITS_PUB_ENDPOINT_KEY "ITS_GBUS_BSCP_ENDPOINT"
#define ITS_SUB_ENDPOINT_KEY "ITS_GBUS_BPCS_ENDPOINT"

#define ITS_GBUS_TOPIC_UPLINK_FRAME "radio.uplink_frame"
#define ITS_GBUS_TOPIC_PA_POWER "radio.pa_power_request"
#define ITS_GBUS_TOPIC_DOWNLINK_FRAME "radio.downlink_frame"
#define ITS_GBUS_TOPIC_UPLINK_STATE "radio.uplink_state"
#define ITS_GBUS_TOPIC_RSSI_INSTANT "radio.rssi_instant"
#define ITS_GBUS_TOPIC_RSSI_PACKET "radio.rssi_packet"
#define ITS_GBUS_TOPIC_RADIO_STATS "radio.stats"



typedef struct timestamp_t
{
	uint64_t seconds;
	uint32_t microseconds;
} timestamp_t;


typedef enum now_topic_t {
	TOPIC_FRAME,
	TOPIC_PA_POWER,
	TOPIC_INVALID
} now_topic_t;


#define TIMESTAMP_S_PRINT_FMT PRIu64
#define TIMESTAMP_uS_PRINT_FMT PRIu32


//! Текущее мировое время
static struct timestamp_t _get_world_time(void)
{
	struct timespec tsc;
	int rc = clock_gettime(CLOCK_REALTIME, &tsc);
	assert(0 == rc);

	struct timestamp_t retval = {
			.seconds = tsc.tv_sec,
			.microseconds = tsc.tv_nsec / 1000
	};
	return retval;
}


//! Разбор метаданных входящего TX фрейма
static int _parse_tx_pa_power_metadata(
		const char * json_buffer, size_t buffer_size, int8_t * pa_power
)
{
	jsmn_parser parser;
	jsmn_init(&parser);

	jsmntok_t t[3];
	int parsed_tokens = jsmn_parse(&parser, json_buffer, buffer_size, t, sizeof(t)/sizeof(*t));
	if (parsed_tokens != 3)
	{
		log_error(
				"invalid tx meta json \"%s\", expected json in form \"{ \"pa_power\": <number> }\": %d",
				parsed_tokens
		);
		return -1;
	}

	if (t[0].type != JSMN_OBJECT || t[1].type != JSMN_STRING || t[2].type != JSMN_PRIMITIVE)
	{
		log_error(
				"invalid pa_power json token types: %d, %d, %d",
				(int)t[0].type, (int)t[1].type, (int)t[2].type
		);
		return -1;
	}

	const jsmntok_t * key_tok = &t[1];
	const char expected_key[] = "pa_power";
	const size_t expected_key_size = sizeof(expected_key) - 1;
	if (expected_key_size != key_tok->end - key_tok->start)
	{
		log_error("tx meta json: pa_power tooken len mismatch");
		return -1;
	}

	if (0 != strncmp(json_buffer + key_tok->start, expected_key, expected_key_size))
	{
		log_error("invalid pa_power json key");
		return -1;
	}


	const jsmntok_t * value_tok = &t[2];
	const char * const value_str_begin_ptr = json_buffer + value_tok->start;
	char * value_str_end_ptr;

	int8_t value = strtoull(value_str_begin_ptr, &value_str_end_ptr, 0);
	if (value_str_end_ptr != json_buffer + value_tok->end)
	{
		log_error("unable to parse tx message pa_power from json metadata");
		return -1;
	}

	*pa_power = value;
	return 0;
}


//! Разбор метаданных входящего TX фрейма
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
				"invalid tx meta json \"%s\", expected json in form \"{ \"cookie\": <number> }\": %d",
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


int zserver_init(zserver_t * zserver)
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

	zserver->zmq = zmq_ctx_new();
	if (!zserver->zmq)
		goto bad_exit;

	zserver->sub_socket = zmq_socket(zserver->zmq, ZMQ_SUB);
	if (!zserver->sub_socket)
	{
		log_error("unable to allocate server sub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	log_info("connecting sub socket to \"%s\"", sub_ep);
	rc = zmq_connect(zserver->sub_socket, sub_ep);
	if (rc < 0)
	{
		log_error("unable to connect server sub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	const char topic[] = ITS_GBUS_TOPIC_UPLINK_FRAME;
	rc = zmq_setsockopt(zserver->sub_socket, ZMQ_SUBSCRIBE, topic, sizeof(topic)-1);
	if (rc < 0)
	{
		log_error("unable to subscribe pub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	{
		const char topic[] = ITS_GBUS_TOPIC_PA_POWER;
		rc = zmq_setsockopt(zserver->sub_socket, ZMQ_SUBSCRIBE, topic, sizeof(topic)-1);
		if (rc < 0)
		{
			log_error("unable to subscribe pub socket: %d, %d: %s", rc, errno, strerror(errno));
			goto bad_exit;
		}
	}

	zserver->pub_socket = zmq_socket(zserver->zmq, ZMQ_PUB);
	if (!zserver->pub_socket)
	{
		log_error("unable to allocate server pub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	log_info("connecting pub socket to \"%s\"", pub_ep);
	rc = zmq_connect(zserver->pub_socket, pub_ep);
	if (rc < 0)
	{
		log_error("unable to connect server pub socket: %d, %d: %s", rc, errno, strerror(errno));
		goto bad_exit;
	}

	return 0;

bad_exit:
	zserver_deinit(zserver);
	return -1;
}


void zserver_deinit(zserver_t * zserver)
{
	if (zserver->sub_socket)
	{
		zmq_close(zserver->sub_socket);
		zserver->sub_socket = NULL;
	}

	if (zserver->pub_socket)
	{
		zmq_close(zserver->pub_socket);
		zserver->pub_socket = NULL;
	}

	if (zserver->zmq)
	{
		int rc = zmq_ctx_destroy(zserver->zmq);
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

		zserver->zmq = NULL;
	}
}


now_topic_t detect_topic(zmq_msg_t * msg)
{

	char topic_buffer[1024] = {0};
	enum now_topic_t now_topic = TOPIC_INVALID;
	const size_t msg_size = zmq_msg_size(msg);

	const char expected_topic[] = ITS_GBUS_TOPIC_UPLINK_FRAME;
	const size_t expected_topic_size = sizeof(expected_topic) - 1;
	if (expected_topic_size == msg_size)
	{
		memcpy(topic_buffer, zmq_msg_data(msg), msg_size);
		if (0 == strncmp(topic_buffer, expected_topic, msg_size))
		{
			now_topic = TOPIC_FRAME;
		}
	}

	if (TOPIC_INVALID == now_topic)
	{
		// Может это не тот топик?
		const char expected_topic[] = ITS_GBUS_TOPIC_PA_POWER;
		const size_t expected_topic_size = sizeof(expected_topic) - 1;
		if (expected_topic_size == msg_size)
		{
			memcpy(topic_buffer, zmq_msg_data(msg), msg_size);
			if (0 == strncmp(topic_buffer, expected_topic, msg_size))
			{
				now_topic = TOPIC_PA_POWER;
			}
			else
			{
				now_topic = TOPIC_INVALID;
			}
		}
	}

	return now_topic;
}


int zserver_recv_tx_packet(
	zserver_t * zserver, uint8_t * buffer, size_t buffer_size,
	size_t * packet_size, msg_cookie_t * packet_cookie, int8_t * packet_pa_power,
	get_message_type_t * message_type
)
{
	int rc;
	enum state_t { STATE_TOPIC, STATE_PA_POWER, STATE_COOKIE, STATE_FRAME, STATE_FLUSH };
	enum state_t state = STATE_TOPIC;

	msg_cookie_t cookie;
	int8_t pa_power;
	zmq_msg_t msg;
	while(1)
	{
		zmq_msg_init(&msg);
		rc = zmq_msg_recv(&msg, zserver->sub_socket, ZMQ_DONTWAIT);
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

			now_topic_t topic = detect_topic(&msg);
			if (TOPIC_INVALID == topic)
				state = STATE_FLUSH;
			else if (TOPIC_FRAME == topic)
				state = STATE_COOKIE;
			else if (TOPIC_PA_POWER == topic)
				state = STATE_PA_POWER;

			// Все ок, работаем дальше

		} break;

		case STATE_PA_POWER: {
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
			rc = _parse_tx_pa_power_metadata(json_buffer, msg_size, &pa_power);
			if (rc < 0)
			{
				// Сообщение об ошибке уже написали
				state = STATE_FLUSH;
				break;
			}

			// Все ок, разобралось
			*packet_pa_power = pa_power;
			*message_type = MESSAGE_PA_POWER;
			state = STATE_FLUSH;
			break;
		}

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
			size_t rcved_size = zmq_msg_size(&msg);
			if (0 == rcved_size)
			{
				log_error("unable to load tx frame. Message have zero size");
				break;
			}

			size_t portion_size = rcved_size;
			if (portion_size > buffer_size)
			{
				log_warn("truncating tx frame from %d to %d", rcved_size, buffer_size);
				portion_size = buffer_size;
			}

			memset(buffer, 0x00, buffer_size);
			memcpy(buffer, zmq_msg_data(&msg), portion_size);
			*packet_size = rcved_size;
			*packet_cookie = cookie;
			*message_type = MESSAGE_FRAME;
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

	return 0;
}


int zserver_send_tx_buffers_state(
	zserver_t * zserver,
	msg_cookie_t cookie_wait, msg_cookie_t cookie_in_progress,
	msg_cookie_t cookie_sent, msg_cookie_t cookie_dropped
)
{
	int rc;

	// Отпрвляем куки TX фреймов, чтобы показать хосту как они раскиданы у нас в буферах
	log_debug(
			"sending tx status. Cookies: "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER" "
			"%"MSG_COOKIE_T_PLSHOLDER,
			cookie_wait,
			cookie_in_progress,
			cookie_sent,
			cookie_dropped
	);

	// Формируем пакет
	// Протокол требует от нас писать null там где нули
	// Поэтому... придется сперва перекинуть в текст те числа, которые не null
	const char json_null[5] = "null";
	char cookie_str_buffers[4][21] = {0}; // 2**64-1 имеет длину 20 симоволов
	msg_cookie_t cookies[] = {
			cookie_wait, cookie_in_progress,
			cookie_sent, cookie_dropped
	};
	for (size_t i = 0; i < sizeof(cookies)/sizeof(*cookies); i++)
	{
		if (0 == cookies[i])
			strcpy(cookie_str_buffers[i], json_null);
		else
		{
			rc = snprintf(
					cookie_str_buffers[i], sizeof(cookie_str_buffers[i]),
					"%"MSG_COOKIE_T_PLSHOLDER, cookies[i]
			);
			if (rc < 0 || rc >= sizeof(cookie_str_buffers[i]))
			{
				log_error("sprintf for cookie no %d failed: %d, %d: %s", i, rc, errno, strerror(errno));
				return 1;
			}
		}
	}

	timestamp_t now;
	now = _get_world_time();

	// Теперь наконец-то sprintf самого json-а
	char json_buffer[1024] = {0};
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"\"time_s\": %"TIMESTAMP_S_PRINT_FMT", "
				"\"time_us\": %"TIMESTAMP_uS_PRINT_FMT", "
				"\"cookie_in_wait\": %s, "
				"\"cookie_in_progress\": %s, "
				"\"cookie_sent\": %s, "
				"\"cookie_dropped\": %s"
			"}",
			now.seconds,
			now.microseconds,
			cookie_str_buffers[0],
			cookie_str_buffers[1],
			cookie_str_buffers[2],
			cookie_str_buffers[3]
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("sprintf for tx_state json failed: %d, %d: %s", rc, errno, strerror(errno));
		return 2;
	}


	// Шлем топик
	const char topic[] = ITS_GBUS_TOPIC_UPLINK_STATE;
	rc = zmq_send(zserver->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status topic: %d: %s", errno, strerror(errno));
		return 3;
	}

	// Шлем данные
	rc = zmq_send(zserver->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send tx status message: %d: %s", errno, strerror(errno));
		return 4;
	}

	return 0;
}


int zserver_send_packet_rssi(
	zserver_t * zserver,
	msg_cookie_t packet_cookie,
	int8_t rssi_pkt, int8_t snr_pkt, int8_t signal_rssi_pkt
)
{
	int rc;

	timestamp_t now;
	now = _get_world_time();

	// Готовим сообщение
	char json_buffer[1024] = {0};
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"\"time_s\": %"TIMESTAMP_S_PRINT_FMT", "
				"\"time_us\": %"TIMESTAMP_uS_PRINT_FMT", "
				"\"cookie\": %"MSG_COOKIE_T_PLSHOLDER", "
				"\"rssi_pkt\": %d, "
				"\"snr_pkt\": %d, "
				"\"rssi_signal\": %d"
			"}",
			now.seconds,
			now.microseconds,
			packet_cookie,
			(int)rssi_pkt,
			(int)snr_pkt,
			(int)signal_rssi_pkt
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("unable to sprintf packet rssi value: %d, %d: %s", rc, errno, strerror(errno));
		return 1;
	}

	// Топик
	const char topic[] = ITS_GBUS_TOPIC_RSSI_PACKET;
	rc = zmq_send(zserver->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("sprintf rx rssi json failed: %d, %d: %s", rc, errno, strerror(errno));
		return 2;
	}

	rc = zmq_send(zserver->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx rssi data: %d: %s", errno, strerror(errno));
		return 3;
	}

	return 0;
}


int zserver_send_rx_packet(
	zserver_t * zserver,
	const uint8_t * packet_data, size_t packet_data_size,
	msg_cookie_t packet_cookie, const uint16_t * packet_no,
	bool crc_valid,	int8_t rssi_pkt, int8_t snr_pkt, int8_t signal_rssi_pkt
)
{
	int rc;
	log_debug("sending rx data");

	timestamp_t now;
	now = _get_world_time();

	char frame_no_buffer[5+1] = { 0 }; // максимум это 65535, в 5 символов влезет + 1 на \0
	const char * frame_no_ptr = "null"; // пока что считаем что оно null
	if (packet_no != NULL)
	{
		rc = snprintf(frame_no_buffer, sizeof(frame_no_buffer), "%"PRIu16"", *packet_no);
		if (rc < 0 || rc >= sizeof(frame_no_buffer))
			log_error("unable to printf packet_number! %d, %d:%s", rc, errno, strerror(errno));
		else
			frame_no_ptr = frame_no_buffer;
	}

	// Готовим метаданные
	char json_buffer[1024] = {0};
	rc = snprintf(
			json_buffer, sizeof(json_buffer),
			"{"
				"\"time_s\": %"TIMESTAMP_S_PRINT_FMT", "
				"\"time_us\": %"TIMESTAMP_uS_PRINT_FMT", "
				"\"checksum_valid\": %s, "
				"\"cookie\": %"MSG_COOKIE_T_PLSHOLDER", "
				"\"frame_no\": %s, "
				"\"rssi_pkt\": %d, "
				"\"snr_pkt\": %d, "
				"\"rssi_signal\": %d"
			"}",
			now.seconds,
			now.microseconds,
			crc_valid ? "true" : "false",
			packet_cookie, frame_no_ptr,
			rssi_pkt,
			snr_pkt,
			signal_rssi_pkt
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("unable to sprintf rx data meta json: %d, %d: %s", rc, errno, strerror(errno));
		return 1;
	}


	// Шлем топик
	const char topic[] = ITS_GBUS_TOPIC_DOWNLINK_FRAME;
	rc = zmq_send(zserver->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data topic: %d: %s", errno, strerror(errno));
		return 2;
	}

	// метаданные
	rc = zmq_send(zserver->pub_socket, json_buffer, strlen(json_buffer), ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx metadata: %d, %d: %s", rc, errno, strerror(errno));
		return 3;
	}

	// Теперь данные самого фрейма
	rc = zmq_send(zserver->pub_socket, packet_data, packet_data_size, ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rx data: %d: %s", errno, errno);
		return 4;
	}

	return 0;
}


int zserver_send_stats(
	zserver_t * zserver, const sx126x_stats_t * stats, uint16_t device_errors,
	const server_stats_t * server_stats
){
	int rc;

	timestamp_t now;
	now = _get_world_time();

	char json_buffer[1024] = { 0 };
	rc = snprintf(
		json_buffer, sizeof(json_buffer),
		"{"
			"\"time_s\": %"TIMESTAMP_S_PRINT_FMT", "
			"\"time_us\": %"TIMESTAMP_uS_PRINT_FMT", "
			"\"pkt_received\": %"PRIu16", "
			"\"crc_errors\": %"PRIu16", "
			"\"hdr_errors\": %"PRIu16", "
			"\"error_rc64k_calib\": %s, "
			"\"error_rc13m_calib\": %s, "
			"\"error_pll_calib\": %s, "
			"\"error_adc_calib\": %s, "
			"\"error_img_calib\": %s, "
			"\"error_xosc_start\": %s, "
			"\"error_pll_lock\": %s, "
			"\"error_pa_ramp\": %s, "
			"\"srv_rx_done\": %"PRIu32", "
			"\"srv_rx_frames\": %"PRIu32", "
			"\"srv_tx_frames\": %"PRIu32", "
			"\"current_pa_power\": %"PRId8", "
			"\"requested_pa_power\": %"PRId8""
		"}",
		now.seconds,
		now.microseconds,
		stats->pkt_received, stats->crc_errors, stats->hdr_errors,
		device_errors & SX126X_DEVICE_ERROR_RC64K_CALIB	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_RC13M_CALIB	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_PLL_CALIB	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_ADC_CALIB	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_IMG_CALIB	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_XOSC_START	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_PLL_LOCK	? "true": "false",
		device_errors & SX126X_DEVICE_ERROR_PA_RAMP		? "true": "false",
		server_stats->rx_done_counter, server_stats->rx_frame_counter, server_stats->tx_frame_counter,
		server_stats->current_pa_power,
		server_stats->requested_pa_power
	);
	if (rc < 0 || rc >= sizeof(json_buffer))
	{
		log_error("spintf radio stats json failed: %d", rc);
		return 1;
	}

	// Топик
	const char topic[] = ITS_GBUS_TOPIC_RADIO_STATS;
	rc = zmq_send(zserver->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send radio stats topic: %d: %s", errno, strerror(errno));
		return 2;
	}

	// Метаданные
	rc = zmq_send(zserver->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send radio stats data: %d: %s", errno, strerror(errno));
		return 3;
	}

	return 0;
}


int zserver_send_instant_rssi(zserver_t * zserver, int8_t rssi)
{
	int rc;
	log_debug("sending rssi %d", (int)rssi);

	timestamp_t now;
	now = _get_world_time();

	// Готовим сообщение
	char json_buffer[1024] = {0};
	rc = snprintf(json_buffer, sizeof(json_buffer),
			"{"
				"\"time_s\": %"TIMESTAMP_S_PRINT_FMT", "
				"\"time_us\": %"TIMESTAMP_uS_PRINT_FMT", "
				"\"rssi\": %d"
			"}",
			now.seconds,
			now.microseconds,
			(int)rssi
	);
	if (rc < 0)
	{
		log_error("sprintf rssi failed: %d, %d: %s", rc, errno, strerror(errno));
		return 1;
	}

	// Топик
	const char topic[] = ITS_GBUS_TOPIC_RSSI_INSTANT;
	rc = zmq_send(zserver->pub_socket, topic, sizeof(topic)-1, ZMQ_SNDMORE | ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi topic: %d: %s", errno, strerror(errno));
		return 2;
	}

	// Само значение
	rc = zmq_send(zserver->pub_socket, json_buffer, strlen(json_buffer), ZMQ_DONTWAIT);
	if (rc < 0)
	{
		log_error("unable to send rssi data: %d: %s", errno, strerror(errno));
		return 3;
	}

	return 0;
}



