/*
 * ark_time_sync.c
 *
 *  Created on: 13 апр. 2020 г.
 *      Author: sereshotes
 */

#include "ark_time_sync.h"
#include "time_sync.h"
#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"

#include "mavlink/its/mavlink.h"
#include "driver/gpio.h"

#include "log_collector.h"
#include <sys/time.h>
#include "init_helper.h"
#include "pinout_cfg.h"
#include "imi.h"


typedef struct {
	uint32_t count;
	struct timeval last_sent;
	int64_t last_when;
	uint8_t base;
} imi_sync_stats_t;

int ark_tsync_send_signal(uint8_t *data, size_t *size, imi_sync_stats_t *stats) {
	struct timeval tp;

	TickType_t start = xTaskGetTickCount();
	gpio_set_level(ITS_PIN_I2CTM_TIME, 0);
	gettimeofday(&tp, 0);

	mavlink_timestamp_t mts;
	mavlink_message_t msg;

	int64_t now = esp_timer_get_time();
	mts.time_s = tp.tv_sec;
	mts.time_us = tp.tv_usec;
	mts.time_base = time_sync_get_base();
	mts.time_steady = (uint32_t) now / 1000;

	stats->last_sent.tv_sec = mts.time_s;
	stats->last_sent.tv_usec = mts.time_us;
	stats->count++;
	stats->last_when = now;
	stats->base = mts.time_base;

	mavlink_msg_timestamp_encode(CUBE_1_BCU, 0, &msg, &mts);
	assert (*size >= mavlink_max_message_length(&msg));
	*size = mavlink_msg_to_send_buffer(data, &msg);
	vTaskDelayUntil(&start, ARK_SIGNAL_LENGTH / portTICK_RATE_MS);

	gpio_set_level(ITS_PIN_I2CTM_TIME, 1);
	return 0;
}


void ark_tsync_task(void *pvParametres) {
	int is_updated = 0;
	imi_sync_stats_t stats = {0};
	while (1) {
		uint8_t packet[MAVLINK_MAX_PACKET_LEN];
		size_t size = sizeof(packet);
		ark_tsync_send_signal(packet, &size, &stats);
		if (imi_send_all(ITS_IMI_PORT, packet, size, 100 / portTICK_RATE_MS)) {
			ESP_LOGI("TIME", "synced i2c devices");
		} else {
			ESP_LOGE("TIME", "can't sync i2c devices");
		}


		if (is_updated) {
			if (log_collector_take(0) == pdTRUE) {
				is_updated = 0;
				mavlink_bcu_time_sync_stats_t *mbtss = log_collector_get_time_sync();
				mbtss->imi_count_of_syncs = stats.count;
				mbtss->imi_time_s = stats.last_sent.tv_sec;
				mbtss->imi_time_us = stats.last_sent.tv_usec;
				mbtss->imi_time_sync_steady = stats.last_when / 1000;
				mbtss->imi_time_base = stats.base;

				log_collector_give();
			}
		}
		vTaskDelay(ARK_TIME_SYNC_PRIOD / portTICK_RATE_MS);
	}
}
