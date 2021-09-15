/*
 * send.c
 *
 *  Created on: Jul 24, 2020
 *      Author: sereshotes
 */


#include "radio.h"
#include "../Inc_private/radio_help.h"
#include "init_helper.h"
#include "router.h"
#include "assert.h"

#include "pinout_cfg.h"
#include "log_collector.h"

#define RADIO_SEND_DELAY 50
#define RADIO_SLEEP_AWAKE_LEGNTH 300 //ms
#define RADIO_SLEEP_SLEEP_LENGTH 4000 //ms
#define MAX_ERRORS 5
#define RADIO_PACKET_PERIOD ((RADIO_TX_PERIOD + RADIO_RX_PERIOD) / RADIO_TX_COUNT)

static radio_t radio_server;


static void _radio_deinit(radio_t * server)
{
	sx126x_drv_t * const radio = &server->dev;

	// резетим радио на последок
	// Даже если ничего не получается
	sx126x_drv_reset(radio);
	// Деструктим дескриптор
	sx126x_drv_dtor(radio);
}


static int _radio_init(radio_t * server)
{
	sx126x_drv_t * const radio = &server->dev;
	int rc;

	const sx126x_drv_basic_cfg_t basic_cfg = {
			.use_dio3_for_tcxo = true,
			.tcxo_v = SX126X_TCXO_CTRL_1_8V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.standby_mode = SX126X_STANDBY_XOSC
	};

	const sx126x_drv_lora_modem_cfg_t modem_cfg = {
			// Параметры усилителей и частот
			.frequency = 438125*1000,
			.pa_ramp_time = SX126X_PA_RAMP_1700_US,
			.pa_power = 17,
			.lna_boost = true,

			// Параметры пакетирования
			.spreading_factor = SX126X_LORA_SF_8,
			.bandwidth = SX126X_LORA_BW_250,
			.coding_rate = SX126X_LORA_CR_4_8,
			.ldr_optimizations = false,
	};

	const sx126x_drv_lora_packet_cfg_t packet_cfg = {
			.invert_iq = false,
			.syncword = SX126X_LORASYNCWORD_PRIVATE,
			.preamble_length = 8,
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
			.stop_timer_on_preamble = false,
			.lora_symb_timeout = 0
	};


	rc = sx126x_drv_ctor(radio, NULL);

	if (0 != rc)
		goto bad_exit;

	rc = sx126x_drv_reset(radio);
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

	rc = sx126x_drv_mode_standby_rc(radio);
	if (0 != rc)
		goto bad_exit;

	return 0;

bad_exit:
	log_error("BAD INIT");
	_radio_deinit(server);
	return -1;
}

static void _save_logs(radio_t *server) {


	if (log_collector_take(0) == pdTRUE) {
		ESP_LOGV("radio", "logs");
		mavlink_bcu_radio_conn_stats_t *conn_stats = log_collector_get_conn_stats();

		sx126x_drv_state_t drv_state = 0;
		sx126x_stats_t drv_stats = {0};
		sx126x_lora_packet_status_t drv_packet = {0};
		uint16_t drv_bits;

		sx126x_drv_get_stats(&server->dev, &drv_stats);
		sx126x_drv_lora_packet_status(&server->dev, &drv_packet);
		sx126x_drv_get_device_errors(&server->dev, &drv_bits);
		drv_state = sx126x_drv_state(&server->dev);

		conn_stats->last_packet_time = server->stats.last_packet_time;
		conn_stats->last_packet_rssi = drv_packet.rssi_pkt;
		conn_stats->last_packet_signal_rssi = drv_packet.signal_rssi_pkt;
		conn_stats->last_packet_snr = drv_packet.snr_pkt;

		conn_stats->count_packet_recieved = drv_stats.pkt_received;
		conn_stats->count_crc_errors = drv_stats.crc_errors;
		conn_stats->count_hdr_errors = drv_stats.hdr_errors;
		conn_stats->count_mavlink_msgs_recieved = server->stats.count_recieved_mavlink_msgs;

		conn_stats->radio_error_bits = drv_bits;
		conn_stats->radio_state = drv_state;
		conn_stats->update_time_radio = esp_timer_get_time() / 1000;
		log_collector_give();
	}
}

typedef enum {
	RS_NONE,
	RS_TX,
	RS_RX,
} radio_trans_dir_state_t;






typedef struct {
	int packet_done;
	int i_really_want_to_start_now;
	int64_t last_sent;
	int64_t last_added;
	int64_t period_start;
	uint32_t error_count_tx;
	uint32_t error_count_rx;
	radio_trans_dir_state_t dir_state;
} radio_private_state_t;


static void _process_event(radio_t *server, sx126x_drv_evt_t event, radio_private_state_t *state, int64_t now) {
	int rc = 0;
	switch (event.kind)
	{
	case SX126X_DRV_EVTKIND_RX_DONE:
		if (!event.arg.rx_done.timed_out) {
			log_info("rx done");

			uint8_t buf[ITS_RADIO_PACKET_SIZE] = {0};
			rc = sx126x_drv_payload_read(&server->dev, buf, sizeof(buf));
			if (0 != rc)
			{
				log_error("unable to read frame from radio buffer: %d", rc);
				return;
			}
			server->stats.last_packet_time = esp_timer_get_time() / 1000;
			if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
				for (int i = 0; i < sizeof(buf); i++) {
					printf("0x%02X ", buf[i]);
				}
				printf("\n");
			}
			mavlink_message_t msg = {0};
			mavlink_status_t st = {0};
			for (int i = 0; i < sizeof(buf); i++) {
				if (mavlink_parse_char(server->mavlink_chan, buf[i], &msg, &st)) {
					log_info("yay! we've got a message %d", msg.msgid);

					server->stats.count_recieved_mavlink_msgs++;

					its_rt_sender_ctx_t ctx = {0};
					its_rt_route(&ctx, &msg, SERVER_SMALL_TIMEOUT_MS);
				}
			}
		} else {
			log_info("rx timeout");
		}
		state->packet_done = 1;
		break;

	case SX126X_DRV_EVTKIND_TX_DONE:
		if (event.arg.tx_done.timed_out) {
			state->error_count_tx++;
			log_error("TX TIMED OUT!");
		} else {
			state->error_count_tx = 0;
			log_info("tx done");
			rbuf_pull(server);
		}
		state->packet_done = 1;
		state->last_sent = now;
		break;
	default:
		log_trace("no event");
		break;
	}
}

static void _update_state(radio_private_state_t *state, radio_t *server, int64_t now) {

	if (state->dir_state == RS_RX && RADIO_RX_PERIOD < now - state->period_start && state->packet_done) {
		log_info("START TX0");
		state->dir_state = RS_TX;
		state->period_start = now;
		rbuf_reset(server);
	}
	if (state->dir_state == RS_TX && RADIO_TX_PERIOD < now - state->period_start && state->packet_done) {
		log_info("START RX");
		state->dir_state = RS_RX;
		state->period_start = now;
	}
	if (state->dir_state == RS_NONE) {
		log_info("START TX2");
		state->dir_state = RS_TX;
		state->period_start = now;
	}
	if (RADIO_START_ANYWAY < now - state->last_sent) {
		log_warn("waited too long. Start anyway");
		log_info("START TX3");
		state->dir_state = RS_TX;
		state->period_start = now;
		state->i_really_want_to_start_now = 1;
		state->last_sent = now;
		state->error_count_tx += MAX_ERRORS / 2 + 1;
		rbuf_reset(server);
	}
}

static void _try_to_send_or_recv(radio_t *server, radio_private_state_t *state, int64_t now) {
	int rc = 0;
	ESP_LOGV("radio", "TEST: %d %d %d %d", (int)state->last_sent, (int)now, state->packet_done, state->dir_state);
	/*
	 * Надо принять/получить пакет? Если предыдущий пакет закончил отправку/получение
	 * и прошло достаточно времени, то надо отправить/получить.
	 * Иногда мы очень хотим отправить. Например, при каком-то зависании. В этом случае
	 * state->i_really_want_to_start_now становится равным единице.
	 */
	if ((state->packet_done && now - state->last_sent >= 000 * 1000) || state->i_really_want_to_start_now) {

		state->packet_done = 0;
		state->i_really_want_to_start_now = 0;

		if (state->dir_state == RS_TX) {
			ESP_LOGV("radio", "new tx");
			radio_buf_t *buf = rbuf_get(server);
			if (LOG_LOCAL_LEVEL >= ESP_LOG_VERBOSE) {
				for (int i = 0; i < buf->size; i++) {
					printf("0x%02X ", buf->buf[i]);
				}
				printf("\n");
				fflush(stdout);
			}
			sx126x_drv_payload_write(&server->dev, buf->buf, buf->size);
			rc = sx126x_drv_mode_tx(&server->dev, 0);
			if (0 != rc) {
				state->error_count_tx++;
				log_error("unable to switch radio to tx mode: %d. Dropping frame", rc);
				state->i_really_want_to_start_now = 1;
			} else {
				log_info("sending packet...");
			}
		}
		if (state->dir_state == RS_RX) {
			ESP_LOGV("radio", "new rx");
			rc = sx126x_drv_mode_rx(&server->dev, RADIO_RX_PERIOD / 1000);
			if (0 != rc) {
				state->error_count_rx++;
				log_error("unable to switch radio to rx mode: %d. Dropping frame", rc);
				state->i_really_want_to_start_now = 1;
			} else {
				log_info("receiving packet...");
			}
		}
	}
}

static void radio_loop(void *arg) {
	radio_t * server = arg;
	// Конфигурация отправки пакетов с контроллируемой скоростью

	// Регистрация таска в маршрутизаторе
	its_rt_task_identifier tid = {
			.name = "radio_send"
	};
	//Регистрируем на сообщения всех типов
	tid.queue = xQueueCreate(20, MAVLINK_MAX_PACKET_LEN);
	its_rt_register_for_all(tid);
	radio_private_state_t state = {0};
	int64_t now = esp_timer_get_time();

	while (1) {
		now = esp_timer_get_time();
		mavlink_message_t incoming_msg = {0};
		while (pdTRUE == xQueueReceive(tid.queue, &incoming_msg, 0))
		{
			// Если мы получили сообщение - складываем в его хранилище
			update_msg(&incoming_msg);
		}
		if (RADIO_PACKET_PERIOD < now - state.last_added) {
			if (rbuf_fill(server)) {
				state.last_added = now;
			}
		}

		sx126x_drv_evt_t event;
		ESP_LOGV("radio", "poll");
		int rc = sx126x_drv_poll_event(&server->dev, &event);
		if (rc) {
			ESP_LOGE("radio", "poll %d", rc);
		}
		if (0 == rc) {
			_process_event(server, event, &state, now);
		}
		if (state.error_count_tx > MAX_ERRORS && state.error_count_rx > MAX_ERRORS) {
			goto big_error;
		}

		_update_state(&state, server, now);

		_try_to_send_or_recv(server, &state, now);

		_save_logs(server);


		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
big_error:
	ESP_LOGE("radio", "too many errors. Restarting...");
	return;
}
static void radio_task(void *arg) {
	int rc = 0;
	while (1) {
		rc = _radio_init(&radio_server);
		if (rc) {
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			continue;
		}

		radio_loop(arg);
	}
}

static TaskHandle_t task_s = NULL;


void radio_send_suspend(void) {
	if (task_s != NULL)
		vTaskSuspend(task_s);
}


void radio_send_resume(void) {
	if (task_s != NULL)
		vTaskResume(task_s);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	TaskHandle_t *handler = arg;
	xTaskNotify(*handler, 0, eNoAction);
}

void radio_send_init(void) {
	radio_server.mavlink_chan = mavlink_claim_channel();
	radio_server.mav_buf.capacity = MAVLINK_MAX_PACKET_LEN;
	for (int i = 0; i < RADIO_TX_COUNT; i++) {
		radio_server.radio_buf[i].capacity = ITS_RADIO_PACKET_SIZE;
		radio_server.radio_buf[i].size = ITS_RADIO_PACKET_SIZE;
	}

	xTaskCreatePinnedToCore(radio_task, "Radio send", configMINIMAL_STACK_SIZE + 4000, &radio_server, 4, &task_s, tskNO_AFFINITY);

/*

	int rc = _radio_init(&radio_server);
	assert(rc == 0);

	xTaskCreatePinnedToCore(task_send, "Radio send", configMINIMAL_STACK_SIZE + 4000, &radio_server, 4, &task_s, tskNO_AFFINITY);
*/

	gpio_isr_handler_add(ITS_PIN_RADIO_DIO1, gpio_isr_handler, (void*) &task_s);
}
