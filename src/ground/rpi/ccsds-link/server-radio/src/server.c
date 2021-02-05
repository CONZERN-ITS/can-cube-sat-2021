#include "server.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <zmq.h>

#include <sx126x_board_rpi.h>


static void _event_handler(
		sx126x_drv_t * drv,
		void * user_arg,
		sx126x_evt_kind_t kind,
		const sx126x_evt_arg_t * arg
);



static void _zmq_deinit(server_t * server)
{
	if (server->data_socket)
	{
		zmq_close(server->data_socket);
		server->data_socket = NULL;
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


	server->data_socket = zmq_socket(server->zmq, ZMQ_PAIR);
	if (!server->data_socket)
	{
		perror("unable to allocate server data socket");
		goto bad_exit;
	}

	rc = zmq_bind(server->data_socket, SERVER_DATA_SOCKET_EP);
	if (rc < 0)
	{
		perror("unable to bind server data socket");
		goto bad_exit;
	}


	server->telemetry_socket = zmq_socket(server->zmq, ZMQ_PUB);
	if (!server->telemetry_socket)
	{
		perror("unable to allocate server telemetry socket");
		goto bad_exit;
	}

	rc = zmq_bind(server->telemetry_socket, SERVER_TELEMETRY_SOCKET_EP);
	if (rc < 0)
	{
		perror("unable to bind server telemetry socket");
		goto bad_exit;
	}

	return 0;

bad_exit:
	_zmq_deinit(server);
	return -1;
}


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

	rc = sx126x_drv_register_event_handler(radio, _event_handler, server);
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


static void _load_tx_packet(server_t * server)
{
	int rc;
	enum state_t { STATE_COOKIE, STATE_FRAME, STATE_FLUSH };
	enum state_t state = STATE_COOKIE;

	uint16_t cookie;
	zmq_msg_t msg;
	while(1)
	{
		zmq_msg_init(&msg);
		rc = zmq_msg_recv(&msg, server->data_socket, ZMQ_DONTWAIT);
		if (rc < 0)
		{
			if (EAGAIN != errno)
				perror("unable to get message from data socket");

			zmq_msg_close(&msg);
			break;
		}

		switch (state)
		{
		case STATE_COOKIE: {
			// Мы сейчас копируем куку сообщения
			if (sizeof(cookie) > zmq_msg_size(&msg))
			{
				state = STATE_FLUSH;
				break;
			}

			memcpy(&cookie, zmq_msg_data(&msg), sizeof(cookie));
			state = STATE_FRAME;
			} break;

		case STATE_FRAME: {
			// Мы сейчас копируем само сообщение
			size_t frame_size = zmq_msg_size(&msg);
			if (frame_size > server->tx_buffer_capacity)
				frame_size = server->tx_buffer_capacity;

			memset(server->tx_buffer, 0x00, server->tx_buffer_capacity);
			memcpy(server->tx_buffer, zmq_msg_data(&msg), frame_size);
			server->tx_buffer_size = frame_size;
			server->tx_cookie_wait = cookie;
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


bool _try_go_tx(server_t * server)
{
	int rc;

	if (0 == server->tx_cookie_wait)
		return false;

	rc = sx126x_drv_payload_write(&server->dev, server->tx_buffer, server->tx_buffer_size);
	if (0 != rc)
	{
		printf("unable to write tx payload to radio: %d. Dropping frame\n", rc);
		server->tx_cookie_dropped = server->tx_cookie_wait;
		server->tx_cookie_wait = 0;
		return false;
	}

	rc = sx126x_drv_mode_tx(&server->dev, server->tx_timeout_ms);
	if (0 != rc)
	{
		printf("unable to switch radio to TX mode: %d. Dropping frame\n", rc);
		server->tx_cookie_dropped = server->tx_cookie_wait;
		server->tx_cookie_wait = 0;
		return false;
	}

	server->tx_cookie_in_progress = server->tx_cookie_wait;
	server->tx_cookie_wait = 0;
	return true;
}


static void _go_rx(server_t * server)
{
	int rc = sx126x_drv_mode_rx(&server->dev, RADIO_RX_TIMEOUT_MS);
	if (0 != rc)
		printf("unable to switch radio to rx!\n");
}


static void _fetch_rx_frame(server_t * server)
{
	int rc;

	rc = sx126x_drv_payload_read(&server->dev, server->rx_buffer, server->rx_buffer_capacity);
	if (0 != rc)
	{
		printf("unable to read frame from radio buffer\n");
		return;
	}

	server->tx_buffer_size = server->rx_buffer_capacity;
}


static void _upload_rx_and_stats(server_t * server)
{
	int rc;

	// Отпрвляем куки TX фреймов, чтобы показать хосту как они раскиданы у нас в буферах
	msg_cookie_t cookies[] = {
			server->tx_cookie_wait,
			server->tx_cookie_in_progress,
			server->tx_cookie_sent,
			server->tx_cookie_dropped,
	};

	for (size_t i = 0; i < sizeof(cookies)/sizeof(*cookies); i++)
	{
		rc = zmq_send(
				server->data_socket,
				&cookies[i],
				sizeof(cookies[i]),
				ZMQ_DONTWAIT | ZMQ_SNDMORE
		);

		if (rc < 0)
		{
			if (errno != EAGAIN)
				perror("unable to upload tx frame cookie");

			goto end;
		}
	}

	// Отправляем сообщение (даже если его нет)
	rc = zmq_send(
			server->data_socket,
			server->rx_buffer,
			server->rx_buffer_size,
			ZMQ_DONTWAIT
	);
	if (rc < 0)
	{
		if (errno != EAGAIN)
			perror("unable to send rx frame to client");

		goto end;
	}

end:
	server->rx_buffer_size = 0;
	return;
}


static void _event_handler(
		sx126x_drv_t * drv,
		void * user_arg,
		sx126x_evt_kind_t kind,
		const sx126x_evt_arg_t * arg
)
{
	server_t * server = (server_t*)user_arg;

	bool went_tx = false;
	switch (kind)
	{
	case SX126X_EVTKIND_RX_DONE:
		if (arg->rx_done.timed_out)
		{
			// Значит мы ничего не получили и эфир свободный
			server->rx_timedout_cnt++;
			// Это странно конечно, мы уже много раз так так сделали?
			if (server->rx_timedout_cnt >= server->rx_timedout_limit)
			{
				// Мы уже долго ждали, будем отправлять
				went_tx = _try_go_tx(server);
				if (went_tx)
					server->rx_timedout_cnt = 0;
			}
		}
		else
		{
			// Мы получили пакет!
			// Выгружаем его с радио
			_fetch_rx_frame(server);
			// Отправляем на базу
			_upload_rx_and_stats(server);
		}
		break;

	case SX126X_EVTKIND_TX_DONE:
		if (arg->tx_done.timed_out)
		{
			// Ой, что-то пошло не так
			printf("TX TIMED OUT!\n");
			server->tx_cookie_dropped = server->tx_cookie_in_progress;
			server->tx_cookie_in_progress = 0;
		}
		else
		{
			// Все пошло так
			server->tx_cookie_sent = server->tx_cookie_in_progress;
			server->tx_cookie_in_progress = 0;
		}

		// Сообщим о произошедшем клиенту
		_upload_rx_and_stats(server);
		break;
	}

	// Если в результате этого события мы не пошли в TX - уходим в RX
	if (!went_tx)
		_go_rx(server);
}


int server_init(server_t * server)
{
	int rc;
	memset(server, 0x00, sizeof(*server));

	server->rx_timedout_limit = RADIO_RX_TIMEDOUT_LIMIT;
	server->rx_buffer_capacity = RADIO_PACKET_SIZE;
	server->rx_timeout_ms = RADIO_RX_TIMEOUT_MS;

	server->tx_buffer_capacity = RADIO_PACKET_SIZE;
	server->tx_timeout_ms = RADIO_TX_TIMEOUT_MS;

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
	_go_rx(server);

	int radio_fd = sx126x_brd_rpi_get_event_fd(server->dev.api.board);

	zmq_pollitem_t pollitems[2] = {0};
	pollitems[0].fd = radio_fd;
	pollitems[0].events = ZMQ_POLLIN | ZMQ_POLLPRI;

	pollitems[1].socket = server->data_socket;
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
			printf("radio event\n");
			// Ага, что-то прозошло с радио
			rc = sx126x_drv_poll(&server->dev);
			if (0 != rc)
				printf("radio poll error %d\n", rc);

			rc = sx126x_brd_rpi_flush_event(server->dev.api.board);
			if (0 != rc)
				printf("unable to flush radio event\n");
		}

		if (pollitems[1].revents)
		{
			printf("zmq_event\n");
			// Что-то пришло на сокет
			// Это может быть только пакет для отправки
			// Вчитываем его
			_load_tx_packet(server);
		}
	}
}


void server_deinit(server_t * server)
{
	_zmq_deinit(server);
	_radio_deinit(server);
}
