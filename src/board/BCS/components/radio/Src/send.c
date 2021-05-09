/*
 * send.c
 *
 *  Created on: Jul 24, 2020
 *      Author: sereshotes
 */


#include "radio.h"
#include "init_helper.h"
#include "router.h"
#include "assert.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "pinout_cfg.h"

#define RADIO_SEND_DELAY 50
#define RADIO_SLEEP_AWAKE_LEGNTH 300 //ms
#define RADIO_SLEEP_SLEEP_LENGTH 4000 //ms



#define log_error(fmt, ...) ESP_LOGE("radio", fmt, ##__VA_ARGS__)
#define log_trace(fmt, ...) ESP_LOGD("radio", fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...) ESP_LOGW("radio", fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) ESP_LOGI("radio", fmt, ##__VA_ARGS__)

typedef struct  {
	int id; //ID сообщения
	float period; //Период отправки сообщений (в кол-ве сообщений)
	int last; //Номер сообщения, когда был отправлен
	int is_updated; //Обновлен ли после последнего отправления
	mavlink_message_t last_msg; //Сообщение на отправку
} msg_container;

/*
 * Буфер всех заданных сообщений
 */
#define F(__msg_hash, __msg_id, __period) {__msg_id, __period, -__period, 0, {0}},
static const msg_container arr_id[] = {
		RADIO_SEND_ID_ARRAY(F)
};
#undef F

/*
 * Хэш-функция для получения индекса в массиве arr_id для заданного
 * номера сообщения.
 * Работает через switch/case. В теории C компилятор способен
 * оптимизировать это при больших кол-ах case. Поэтому через
 * define определенно switch/case выражение, которое возвращает
 * индекс в массиве arr_id для msgid данного сообщения.
 */
static int get_hash(int id) {
#define F(x,a,b) case a: return x;
	switch(id) {
	RADIO_SEND_ID_ARRAY(F)
	default:
		return -1;
	}
#undef F
}

//! Проверка на то, что сообщение забанено на отправку по радио
/*! Вернет 0, если не забанено и не 0, если забанено */
static int is_msg_banned(int msg_id)
{
#define F(__msg_id) case __msg_id: return 1;
	switch (msg_id) {
	RADIO_SEND_BAN(F)
	default: break;
	}
#undef F

	return 0;
}

#define RADIO_SEND_BUF_SIZE 30
static msg_container arr_buf[RADIO_SEND_BUF_SIZE];
static int arr_buf_size = 0;
static SemaphoreHandle_t buf_mutex;

/*
 * Поиск контейнера для данного сообщения в буфере arr_buf
 */
static msg_container* find(const mavlink_message_t *msg) {
	for (int i = 0; i < arr_buf_size; i++) {
		if (arr_buf[i].last_msg.msgid == msg->msgid &&
			arr_buf[i].last_msg.compid == msg->compid &&
			arr_buf[i].last_msg.sysid == msg->sysid) {
			return  &arr_buf[i];
		}
	}
	return 0;
}
/*
 * Создание контейнера в буфере arr_buf для данного сообщения
 */
static int add(const mavlink_message_t *msg) {

	if (is_msg_banned(msg->msgid)) {
		return 2;
	}

	if (arr_buf_size >= RADIO_SEND_BUF_SIZE) {
		ESP_LOGE("radio", "No free space for new msg");
		return 1;
	}

	ESP_LOGI("radio", "Add: %d %d:%d", msg->msgid, msg->sysid, msg->compid);
	int id = get_hash(msg->msgid);
	if (id >= 0) {
		arr_buf_size++;
		arr_buf[arr_buf_size - 1] = arr_id[id];
	} else {
		arr_buf_size++;
		arr_buf[arr_buf_size - 1].id = msg->msgid;
		arr_buf[arr_buf_size - 1].period = RADIO_DEFAULT_PERIOD;
		arr_buf[arr_buf_size - 1].last = -RADIO_DEFAULT_PERIOD;
	}
	return 0;
}

/*
 * Обновление сообщения в соответствующем контейнере из буфера arr_buf.
 * Если контейнера нет - создает новый.
 */
static void update_msg(const mavlink_message_t *msg) {
	msg_container *st = find(msg);
	if (!st) {
		if (add(msg)) {
			return;
		}
		st = &arr_buf[arr_buf_size - 1];
	}
	st->last_msg = *msg;
	st->is_updated = 1;
}
/*
 * Коэффициент важности/срочности/хорошести данного сообщения.
 * Больше - важнее.
 */
static float get_coef(const msg_container *a, int now) {
	//assert(a->period > 0);
	return (now - a->last) / a->period;
}
/*
 * Отбирает самое важное/срочное/лучшее сообщение для отправки.
 * now - количество отпраленных сообщений до этого момента.
 */
static msg_container *get_best(int now) {
	msg_container *st_best = 0;
	float coef_best = 0;
	for (int i = 0; i < arr_buf_size; i++) {
		float coef = get_coef(&arr_buf[i], now);
		if (coef > coef_best && arr_buf[i].is_updated) {
			st_best = &arr_buf[i];
			coef_best = coef;
		}
	}
	if (st_best) {
		ESP_LOGI("radio", "chosen %d with coef %f, period %f, last %d, now %d",
				st_best->id, coef_best, st_best->period, st_best->last, now);
	}
	return st_best;
}


/*
 * Настройки safe_uart_send
 */
typedef struct  {
	uint32_t baud_send; //Баудрейт устр-ва в бит/сек
	uint32_t buffer_size; //Размер буфера внутри устр-ва
	uart_port_t port; //Порт уарта
	int super_portion_byte_limit;
	int empty_buffer_time_limit;
	int64_t sleep_delay;
} safe_send_cfg_t;

/*
 * Параметры safe_uart_send
 */

typedef enum sleep_state_t
{
	SLEEP_STATE_SENDING,
	SLEEP_STATE_WAIT_ZERO,
	SLEEP_STATE_WAIT_TIME,
} sleep_state_t;



typedef struct  {
	safe_send_cfg_t cfg; //Настройки
	int filled;	//Заполненность буферв
	int64_t last_checked; //Время в мкс последнего изменения filled
	sleep_state_t sleep_state;
	int super_portion_byte_counter;
	int empty_buffer_time_deadline;
} safe_send_t;


static 	safe_send_t sst = {
	.cfg = {
			.baud_send = 2400 / 2,
			.buffer_size = 1000,
			.port = ITS_UARTR_PORT,
			.super_portion_byte_limit = 300,
			.empty_buffer_time_limit = 0,
			.sleep_delay = RADIO_DEFAULT_PERIOD,
	},
	.sleep_state = SLEEP_STATE_SENDING,
	.super_portion_byte_counter = 300
};

/*
static int is_sleeping(safe_send_t *sst) {
	int new_state = sst->sleep_state;
	int64_t now = esp_timer_get_time();
	if (sst->sleep_state == 0) {
		new_state = (now - sst->last_changed_sleep_time) / 1000 > RADIO_SLEEP_AWAKE_LEGNTH ? 1 : 0;
	} else {
		new_state = (now - sst->last_changed_sleep_time) / 1000 > RADIO_SLEEP_SLEEP_LENGTH ? 0 : 1;
	}
	if (new_state != sst->sleep_state) {
		ESP_LOGD("radio", "state: %d %d %d", (int)now, (int)sst->last_changed_sleep_time, (int)sst->sleep_state);
		sst->last_changed_sleep_time = now;
	}
	sst->sleep_state = new_state;
	return sst->sleep_state;
}
*/
int fill_one_packet(radio_t * server) {
/*	static int nat = 0;
	log_info("fill packet");
	for (; server->radio_buf.index < server->radio_buf.size; server->radio_buf.index++) {
		server->radio_buf.buf[server->radio_buf.index] = nat++;
	}*/
	if (server->radio_buf.index < server->radio_buf.size) {
		log_info("gen %d %d", server->radio_buf.index, server->radio_buf.size);
		if (server->mav_buf.size - server->mav_buf.index <= 0) {
			msg_container *st = 0;
			st = get_best(server->msg_count);
			if (0 == st) {
				goto end;
			}

			server->mav_buf.size = mavlink_msg_to_send_buffer(server->mav_buf.buf, &st->last_msg);
			server->mav_buf.index = 0;
			st->is_updated = 0; // Сообщение внутри контейнера уже не свежее
			st->last = server->msg_count;// Запоминаем, когда сообщение было отправленно в последний раз
			server->msg_count++;
		}

		uint8_t *out = server->mav_buf.buf + server->mav_buf.index;
		int cnt = server->mav_buf.size - server->mav_buf.index;
		int diff = server->radio_buf.size - server->radio_buf.index;
		if (cnt > diff) {
			cnt = diff;
		}
		memcpy(server->radio_buf.buf + server->radio_buf.index, out, cnt);
		server->mav_buf.index += cnt;
		server->radio_buf.index += cnt;

	}
end:
	return server->radio_buf.index == server->radio_buf.size;
}


int fill_packet(radio_t * server) {
/*	static int nat = 0;
	log_info("fill packet");
	for (; server->radio_buf.index < server->radio_buf.size; server->radio_buf.index++) {
		server->radio_buf.buf[server->radio_buf.index] = nat++;
	}*/
	while (server->radio_buf.index < server->radio_buf.size) {
		log_info("gen %d %d", server->radio_buf.index, server->radio_buf.size);
		if (server->mav_buf.size - server->mav_buf.index > 0) {
			uint8_t *out = server->mav_buf.buf + server->mav_buf.index;
			int cnt = server->mav_buf.size - server->mav_buf.index;
			int diff = server->radio_buf.size - server->radio_buf.index;
			if (cnt > diff) {
				cnt = diff;
			}
			memcpy(server->radio_buf.buf + server->radio_buf.index, out, cnt);
			server->mav_buf.index += cnt;
			server->radio_buf.index += cnt;
		} else {
			msg_container *st = 0;
			st = get_best(server->msg_count);
			if (0 == st) {
				break;
			}

			server->mav_buf.size = mavlink_msg_to_send_buffer(server->mav_buf.buf, &st->last_msg);
			server->mav_buf.index = 0;
			st->is_updated = 0; // Сообщение внутри контейнера уже не свежее
			st->last = server->msg_count;// Запоминаем, когда сообщение было отправленно в последний раз
			server->msg_count++;
		}
	}
	return server->radio_buf.index == server->radio_buf.size;
}

static void _radio_event_handler(sx126x_drv_t * drv, void * user_arg,
		sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg
);


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
			.tcxo_v = SX126X_TCXO_CTRL_1_6V,
			.use_dio2_for_rf_switch = false,
			.allow_dcdc = true,
			.standby_mode = SX126X_STANDBY_XOSC
	};

	const sx126x_drv_lora_modem_cfg_t modem_cfg = {
			// Параметры усилителей и частот
			.frequency = 434000*1000,
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
	log_error("BAD INIT");
	_radio_deinit(server);
	return -1;
}


static void _radio_event_handler(sx126x_drv_t * drv, void * user_arg,
		sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg
)
{
	int rc;
	log_trace("Event handler");
	radio_t * radio_server = (radio_t*)user_arg;

	bool went_tx = false;
	switch (kind)
	{
	case SX126X_EVTKIND_RX_DONE:
		if (!arg->rx_done.timed_out) {
			log_trace("rx done");

			uint8_t buf[ITS_RADIO_PACKET_SIZE] = {0};
			rc = sx126x_drv_payload_read(&radio_server->dev, buf, sizeof(buf));
			if (0 != rc)
			{
				log_error("unable to read frame from radio buffer: %d", rc);
				return;
			}
			mavlink_message_t msg = {0};
			mavlink_status_t st = {0};
			for (int i = 0; i < sizeof(buf); i++) {
				if (mavlink_parse_char(radio_server->mavlink_chan, buf[i], &msg, &st)) {
					log_trace("yay! we've got a message %d", msg.msgid);
					its_rt_sender_ctx_t ctx = {0};
					its_rt_route(&ctx, &msg, SERVER_SMALL_TIMEOUT_MS);
				}
			}
		} else {
			log_trace("rx timeout");
		}
		radio_server->is_ready_to_send = 1;
		break;

	case SX126X_EVTKIND_TX_DONE:
		if (arg->tx_done.timed_out)
		{
			// Ой, что-то пошло не так
			log_error("TX TIMED OUT!");
		}
		else
		{
			// Все пошло так
			log_info("tx done");
		}

		rc = sx126x_drv_mode_rx(&radio_server->dev, 0);
		if (0 != rc) {
			log_error("unable to switch radio to rx mode: %d. Dropping frame", rc);
			return;
		}
		break;
	default:
		break;
	}
}
static void task_send(void *arg) {
	radio_t * radio_server = arg;
	// Конфигурация отправки пакетов с контроллируемой скоростью

	// Регистрация таска в маршрутизаторе
	its_rt_task_identifier tid = {
			.name = "radio_send"
	};
	//Регистрируем на сообщения всех типов
	tid.queue = xQueueCreate(20, MAVLINK_MAX_PACKET_LEN);
	its_rt_register_for_all(tid);


	//Количество отправленных сообщений
	int msg_count = 0;
	// Буфер для отправляемого сообщения
	uint8_t out_buf[MAVLINK_MAX_PACKET_LEN] = {0};

	// Текущая отправляемая порция
	uint8_t * portion = out_buf;
	// размер порции
	int portion_size = 0;


	sx126x_drv_mode_rx(&radio_server->dev, 0);
	static int64_t last_changed = 0;
	last_changed = esp_timer_get_time();
	while (1) {
		ESP_LOGV("radio", "STEP");
		uint32_t dummy = 0;
		xTaskNotifyWait(0, 0, &dummy, 50 / portTICK_PERIOD_MS);
		int rc = sx126x_drv_poll(&radio_server->dev);

		ESP_LOGV("radio", "poll");
		if (rc) {
			ESP_LOGE("radio", "poll %d", rc);
		}

		ESP_LOGV("radio", "recieving");
		mavlink_message_t incoming_msg = {0};
		while (pdTRUE == xQueueReceive(tid.queue, &incoming_msg, 0))
		{
			// Если мы получили сообщение - складываем в его хранилище
			update_msg	(&incoming_msg);
		}
		int is_filled = 0;
		if (radio_server->is_ready_to_send) {
			is_filled = fill_packet(radio_server);
		} else {
			is_filled = fill_one_packet(radio_server);
		}
		ESP_LOGV("radio", "filled");
		if ((is_filled && radio_server->is_ready_to_send) || esp_timer_get_time() - last_changed > 1000000 * 10) {
			/*log_info("out buf: ");
			for (int i = 0; i < radio_server->radio_buf.size; i++) {
				printf("0x%02X ", radio_server->radio_buf.buf[i]);
			}
			printf("\n");*/

			rc = sx126x_drv_payload_write(&radio_server->dev, radio_server->radio_buf.buf, ITS_RADIO_PACKET_SIZE);
			if (0 != rc) {
				log_error("unable to write tx payload to radio: %d. Dropping frame", rc);
				return;
			}

			ESP_LOGV("radio", "writed");
			rc = sx126x_drv_mode_tx(&radio_server->dev, 0);
			if (0 != rc) {
				log_error("unable to switch radio to tx mode: %d. Dropping frame", rc);
				return;
			}

			ESP_LOGV("radio", "txed");
			radio_server->is_ready_to_send = 0;
			radio_server->radio_buf.index = 0;
			last_changed = esp_timer_get_time();
		}



		ESP_LOGV("radio", "sleep?");

		int64_t now = esp_timer_get_time();

		switch (sst.sleep_state)
		{
		case SLEEP_STATE_SENDING:
			if (sst.super_portion_byte_counter > 0)
			{
				// Квота еще есть, продолжаем
				break;
			}
			ESP_LOGD("radio", "state: %d %d %d %d", sst.super_portion_byte_counter, sst.empty_buffer_time_deadline, sst.sleep_state, sst.filled);
			sst.sleep_state = SLEEP_STATE_WAIT_ZERO;
			continue;

		case SLEEP_STATE_WAIT_ZERO:
			// Ждем пока заполненость буфера опустится до 0
			if (sst.filled <= 0)
			{
				ESP_LOGD("radio", "state: %d %d %d %d", sst.super_portion_byte_counter, sst.empty_buffer_time_deadline, sst.sleep_state, sst.filled);
				// Теперь будем ждать время
				sst.empty_buffer_time_deadline = now + sst.cfg.sleep_delay * 1000;
				sst.sleep_state = SLEEP_STATE_WAIT_TIME;
			}
			continue;

		case SLEEP_STATE_WAIT_TIME:
			if (now >= sst.empty_buffer_time_deadline)
			{
				ESP_LOGD("radio", "state: %d %d %d %d", sst.super_portion_byte_counter, sst.empty_buffer_time_deadline, sst.sleep_state, sst.filled);
				// Переходим в ссстояние отправки
				sst.super_portion_byte_counter = sst.cfg.super_portion_byte_limit;
				sst.sleep_state = SLEEP_STATE_SENDING;
			}
			continue;
		}
	} // while(1)
	vTaskDelete(NULL);
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

void radio_send_set_sleep_delay(int64_t sleep_delay) {
	sst.cfg.sleep_delay = sleep_delay;
}
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	TaskHandle_t *handler = arg;
	xTaskNotify(*handler, 0, eNoAction);
}

static radio_t radio_server;
void radio_send_init(void) {
	radio_server.mavlink_chan = mavlink_claim_channel();
	radio_server.mav_buf.capacity = MAVLINK_MAX_PACKET_LEN;
	radio_server.radio_buf.capacity = ITS_RADIO_PACKET_SIZE;
	radio_server.radio_buf.size = ITS_RADIO_PACKET_SIZE;

	int rc = _radio_init(&radio_server);
	assert(rc == 0);

	xTaskCreatePinnedToCore(task_send, "Radio send", configMINIMAL_STACK_SIZE + 4000, &radio_server, 4, &task_s, tskNO_AFFINITY);
	gpio_isr_handler_add(ITS_PIN_RADIO_DIO1, gpio_isr_handler, (void*) &task_s);
}
