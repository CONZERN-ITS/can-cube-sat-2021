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
#include <stdint.h>
#include <inttypes.h>

#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"
#include "esp_sntp.h"
#include "log_collector.h"

const char *TAG = "TIME_SYNC";

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

typedef struct {
	uint32_t count_total;
	uint32_t count_aggressive;
	struct timeval last_from;
	struct timeval last_to;
	int64_t last_when;
	uint8_t last_from_base;
	uint8_t last_to_base;
} time_sync_stats_t;

static int _try_aggressive_sync(ts_sync *ts, mavlink_message_t *msg, time_sync_stats_t *stats) {
	int is_updated = 0;
	struct timeval there;
	mavlink_timestamp_t mts;
	mavlink_msg_timestamp_decode(msg, &mts);


	if (xSemaphoreTake(ts->mutex, portMAX_DELAY) == pdTRUE) {
		int64_t now_steady = esp_timer_get_time();
		struct timeval now;
		gettimeofday(&now, 0);

		there.tv_sec = mts.time_s;
		there.tv_usec = mts.time_us;
		int64_t diff_usec = now_steady - ts->mutex_safe.isr_time + (there.tv_sec - now.tv_sec) * 1000000ll + (0 - now.tv_usec);

		ESP_LOGV("TIME", "from sinc: %llu.%06d", (uint64_t)there.tv_sec, (int)there.tv_usec);
		ESP_LOGV("TIME", "here:      %llu.%06d", (uint64_t)now.tv_sec, (int)now.tv_usec);
		ESP_LOGV("TIME", "diff:      %d.%06d %lld", (int)(diff_usec / 1000000), (int)(diff_usec % 1000000), now_steady - ts->mutex_safe.isr_time	);
		ESP_LOGV("TIME", "Base: %d %d %d", ts->mutex_safe.base, mts.time_base, ts->cnt);
		if (mts.time_base >= ts->mutex_safe.base) {
			//Разница больше 5 секунд? Меняем!
			if (llabs(diff_usec) > 5 * 1000000ll) {
				ESP_LOGV("TIME", "CHANGE TIME %d", (int)diff_usec);
				int64_t now1 = now.tv_sec * 1000000ll + now.tv_usec + diff_usec;
				stats->last_from = now;
				now.tv_sec = now1 / 1000000;
				now.tv_usec = now1 % 1000000;
				stats->last_to = now;
				settimeofday(&now, 0);
				stats->count_aggressive++;
				stats->count_total++;
				stats->last_when = now_steady;
				stats->last_from_base = ts->mutex_safe.base;
				stats->last_to_base = mts.time_base;
				is_updated = 1;
				ts->last_changed = now.tv_sec * 1000000ull + now.tv_usec;
				ts->cnt = 0;
				ts->diff_total = 0;
				ts->mutex_safe.base = mts.time_base;
				ts->min_collected_base = TIME_BASE_TYPE_GPS;
			} else {
				//Будем накапливать разницу
				ts->diff_total += diff_usec;
				ts->cnt++;
				if (ts->min_collected_base > mts.time_base) {
					ts->min_collected_base = mts.time_base;
				}
			}
		}
		xSemaphoreGive(ts->mutex);
	}
	return is_updated;
}


static int _try_soft_sync(ts_sync *ts, time_sync_stats_t *stats) {
	int is_updated = 0;
	/*
	 * Мы хотим менять время лишь в четко заданные интервалы времени. Если период 5 минут, то мы хотим, чтобы
	 * время менялось в момент, кратный 5 минутам. Поэтому задаем интервал, когда время может меняться.
	 * Также ставим ограниение на разницу по времени между изменениями, чтобы время не менялось несколько раз
	 * за один интервал.
	 */
	struct timeval t;
	gettimeofday(&t, 0);
	uint64_t now = t.tv_sec * 1000000ull + t.tv_usec;
	ESP_LOGV("TIME_SYNC", "now %d.%06d %"PRIu64, (int)t.tv_sec, (int)t.tv_usec, now);
	uint64_t now_low = (uint64_t)(now / ts->cfg.period) * ts->cfg.period;
	uint64_t up_border = now_low + 0.03 * ts->cfg.period;
	uint64_t low_border = now_low + 0.97 * ts->cfg.period;
	if (ts->cnt >= 1 && (now < up_border || now > low_border) && now - ts->last_changed > ts->cfg.period) {
		ESP_LOGV("TIME_SYNC", "now %"PRIu64" up_border %"PRIu64" low_border %"PRIu64" now_low %"PRIu64, now, up_border, low_border, now_low);

		if (xSemaphoreTake(ts->mutex, portMAX_DELAY) == pdTRUE) {
			int64_t estimated = now + ts->diff_total / ts->cnt;
			stats->last_from = t;
			t.tv_sec = estimated / 1000000;
			t.tv_usec = estimated % 1000000;
			stats->last_to = t;
			stats->last_when = esp_timer_get_time();
			stats->count_total++;
			stats->last_from_base = ts->mutex_safe.base;
			stats->last_to_base = ts->min_collected_base;
			settimeofday(&t, 0);
			is_updated = 1;
			ts->last_changed = now;
			ts->cnt = 0;
			ts->diff_total = 0;
			ts->mutex_safe.base = ts->min_collected_base;
			ts->min_collected_base = TIME_BASE_TYPE_GPS;
			ESP_LOGV("TIME_SYNC", "smooth changing time %"PRIu64" %"PRIu64" %d", ts->diff_total, estimated, ts->cnt);
			xSemaphoreGive(ts->mutex);
		}
	}
	return is_updated;
}

static void time_sync_task(void *arg) {
	ts_sync *ts = (ts_sync *)arg;
	time_sync_stats_t stats = {0};

	its_rt_task_identifier id = {
			.name = "time_sync"
	};
	id.queue = xQueueCreate(3, MAVLINK_MAX_PACKET_LEN);

	//Регистрируем очередь для приема сообщений времени
	its_rt_register(MAVLINK_MSG_ID_TIMESTAMP, id);
	int is_updated = 0;
	int is_any_msg = 0;
	while (1) {
		mavlink_message_t msg;
		mavlink_message_t temp_msg;
		//Ждем получения сообщений
		uint32_t ticks_to_wait = is_updated ? 200 / portTICK_PERIOD_MS : portMAX_DELAY;
		is_any_msg = 0;
		if (xQueueReceive(id.queue, &temp_msg, ticks_to_wait) == pdTRUE)  {
			//ESP_LOGV(TAG, "Got message?. Trying to sync");
			//Был ли pps сигнал
			if (ulTaskNotifyTake(pdTRUE, 0) == 0) {
				ESP_LOGI("TIME SYNC","Where is signal?");
				is_any_msg = 0;
			} else {
				do  {
					//ESP_LOGV(TAG, "hmmmm");
					//Пришло ли это от SINS
					if (temp_msg.sysid != CUBE_1_SINS) {
						ESP_LOGI("TIME SYNC","Who is sending it?");
						continue;
					}
					msg = temp_msg;
					is_any_msg = 1;
				} while (xQueueReceive(id.queue, &temp_msg, 0) == pdTRUE);
			}

		}
		if (is_any_msg) {
			//ESP_LOGV(TAG, "Got message. Trying to sync");
			is_updated = _try_aggressive_sync(ts, &msg, &stats) || is_updated;
			is_updated = _try_soft_sync(ts, &stats) || is_updated;
		}

		if (is_updated) {
			if (log_collector_take(0) == pdTRUE) {
				is_updated = 0;
				mavlink_bcu_time_sync_stats_t *mbtss = log_collector_get_time_sync();
				mbtss->count_of_time_updates = stats.count_total;
				mbtss->count_of_aggressive_time_updates = stats.count_aggressive;
				mbtss->updated_time_from_s = stats.last_from.tv_sec;
				mbtss->updated_time_from_us = stats.last_from.tv_usec;
				mbtss->updated_time_from_base = stats.last_from_base;
				mbtss->updated_time_to_s = stats.last_to.tv_sec;
				mbtss->updated_time_to_us = stats.last_to.tv_usec;
				mbtss->updated_time_to_base = stats.last_to_base;
				mbtss->updated_time_steady = stats.last_when / 1000;
				log_collector_give();
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
