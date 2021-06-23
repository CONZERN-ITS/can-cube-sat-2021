/*
 * time_sync.c
 *
 *  Created on: Jul 14, 2020
 *      Author: sereshotes
 */
/*
 * Необходимо иметь службу времени, которая будет
 * синхронизироваться либо с SINS через uart/mavlink и pps,
 * либо с BCS через wifi/ntp. В этом модуле представлены
 * функции для установки службы для любой из этих задач.
 */
#include "time_sync.h"
#include "ntp_server.h"

#include "router.h"
#include "pinout_cfg.h"
#include "mavlink/its/mavlink.h"

#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_sntp.h"

typedef struct {

	int base;
	//struct timeval here;
	int64_t isr_time;
} ts_mutex_safe;

typedef struct {

	ts_cfg cfg;
	ts_mutex_safe mutex_safe;
	xSemaphoreHandle mutex;
	TaskHandle_t task;
	int64_t diff_total;
	int cnt;
	int64_t last_changed;

	uint8_t min_collected_base;

} ts_sync;




//static void adjust(struct timeval *t1, struct timeval *t2)

void sntp_notify(struct timeval *tv);

/*
 *	Таск, синхронизирующийся с SINS, если пришло
 *	от него сообщение и пришел pps сигнал
 */

static ts_sync __ts;

static void time_sync_task(void *arg) {
	ts_sync *ts = (ts_sync *)arg;


	its_rt_task_identifier id = {
			.name = "time_sync"
	};
	id.queue = xQueueCreate(3, MAVLINK_MAX_PACKET_LEN);

	//Регистрируем очередь для приема сообщений времени
	its_rt_register(MAVLINK_MSG_ID_TIMESTAMP, id);
	while (1) {
		mavlink_message_t msg;
		//Ждем получения сообщений
		if (!xQueueReceive(id.queue, &msg, portMAX_DELAY)) {
			//Никогда не должно происходить
			ESP_LOGE("TIME SYNC", "Recieve error");
			vTaskDelay(5000 / portTICK_RATE_MS);
			continue;
		}

		//Пришло ли это от SINS
		if (msg.sysid != CUBE_1_SINS) {
			ESP_LOGI("TIME SYNC","Who is sending it?");
			continue;
		}
		//Был ли pps сигнал
		if (ulTaskNotifyTake(pdTRUE, 0) == 0) {
			ESP_LOGI("TIME SYNC","Where is signal?");
			continue;
		}

		struct timeval there;
		mavlink_timestamp_t mts;
		mavlink_msg_timestamp_decode(&msg, &mts);


		if (xSemaphoreTake(ts->mutex, portMAX_DELAY) == pdTRUE) {
			int64_t now1 = esp_timer_get_time();
			struct timeval now;
			gettimeofday(&now, 0);

			there.tv_sec = mts.time_s;
			there.tv_usec = mts.time_us;
			int64_t diff_usec = now1 - ts->mutex_safe.isr_time + (there.tv_sec - now.tv_sec) * 1000000ll + (0 - now.tv_usec);

			if (mts.time_base >= ts->mutex_safe.base) {
				if (llabs(diff_usec) > 3600 * 1000000ll) {
					settimeofday(&there, 0);
					ts->cnt = 0;
					ts->diff_total = 0;
					ts->mutex_safe.base = mts.time_base;
					ts->min_collected_base = TIME_BASE_TYPE_GPS;
				} else {
					ts->diff_total += diff_usec;
					ts->cnt++;
					if (ts->min_collected_base > mts.time_base) {
						ts->min_collected_base = mts.time_base;
					}
				}
			}
			ESP_LOGV("TIME", "from sinc: %d.%06d", (int)there.tv_sec, (int)there.tv_usec);
			ESP_LOGV("TIME", "here:      %d.%06d", (int)now.tv_sec, (int)now.tv_usec);
			ESP_LOGV("TIME", "diff:      %d.%06d", (int)(diff_usec / 1000000), (int)(diff_usec % 1000000));
			ESP_LOGV("TIME", "Base: %d %d", ts->mutex_safe.base, mts.time_base);
			xSemaphoreGive(ts->mutex);
		}


		/*
		 * Мы хотим менять время лишь в четко заданные интервалы времени. Если период 5 минут, то мы хотим, чтобы
		 * время менялось в момент, кратный 5 минутам. Поэтому задаем интервал, когда время может меняться.
		 * Также ставим ограниение на разницу по времени между изменениями, чтобы время не менялось несколько раз
		 * за один интервал.
		 */
		struct timeval t;
		gettimeofday(&t, 0);
		int64_t now = t.tv_sec * 1000000 + t.tv_usec;
		if (ts->cnt >= 1 &&
				now - ts->last_changed > ts->cfg.period / 2 &&
				now < ((int)(now / ts->cfg.period)) * ts->cfg.period + 0.03 * ts->cfg.period) {

			if (xSemaphoreTake(ts->mutex, portMAX_DELAY) == pdTRUE) {
				int64_t estimated = now + ts->diff_total / ts->cnt;
				t.tv_sec = estimated / 1000000;
				t.tv_usec = estimated % 1000000;
				settimeofday(&t, 0);
				ts->last_changed = now;
				ts->cnt = 0;
				ts->mutex_safe.base = ts->min_collected_base;
				ts->min_collected_base = TIME_BASE_TYPE_GPS;
				xSemaphoreGive(ts->mutex);
			}
		}
	}

}

/*
 * Функция, вызваемая при опускании линии pps
 */
static void IRAM_ATTR isr_handler(void *arg) {
	ts_sync *ts = (ts_sync *)arg;

	BaseType_t higherWoken = 0;
	xSemaphoreTakeFromISR(ts->mutex, &higherWoken);
	ts->mutex_safe.isr_time = esp_timer_get_time();
	//gettimeofday(&ts->mutex_safe.here, 0);
	xSemaphoreGiveFromISR(ts->mutex, &higherWoken);


	vTaskNotifyGiveFromISR(ts->task, &higherWoken);
	if (higherWoken) {
		portYIELD_FROM_ISR();
	}
}

/*
 * Устанавливает службу для синхронизации от SINS
 * через uart/mavlink и pps сигнал. В cfg необходимо
 * инициализировать номер пина.
 */
void time_sync_from_sins_install(ts_cfg *cfg) {
	__ts.cfg = *cfg;
	__ts.mutex = xSemaphoreCreateMutex();
	//xTaskCreatePinnedToCore(ntp_server_task, "SNTP server", configMINIMAL_STACK_SIZE + 4000, 0, 1, 0, tskNO_AFFINITY);

	xTaskCreate(time_sync_task, "timesync", 4096, &__ts, 1, &__ts.task);
	gpio_config_t init_pin_int = {
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_POSEDGE,
		.pin_bit_mask = 1ULL << cfg->pin
	};
	gpio_config(&init_pin_int);
	gpio_isr_handler_add(cfg->pin, isr_handler, &__ts);
}

/*
 * Устанавливает службу для синхронизации от BCS,
 * который имеет точно время. Работает через wifi/ip/ntp
 */
void time_sync_from_bcs_install(const ip_addr_t *server_ip) {
	sntp_set_time_sync_notification_cb(sntp_notify);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setserver(0, server_ip);
	sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
	sntp_set_sync_interval(0);
	sntp_init();
	sntp_restart();
	ESP_LOGI("SNTP", "client started");
}



void sntp_notify(struct timeval *tv) {
	ESP_LOGI("SNTP", "We've just synced");
}

uint8_t time_sync_get_base() {

	volatile uint8_t t;
	xSemaphoreTake(__ts.mutex, portMAX_DELAY);
	ESP_LOGV("TIME", "Base2: %d", __ts.mutex_safe.base);
	t = __ts.mutex_safe.base;
	xSemaphoreGive(__ts.mutex);
	return t;
}
