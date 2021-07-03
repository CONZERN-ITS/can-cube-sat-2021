/*
 * log_collector.c
 *
 *  Created on: Aug 19, 2020
 *      Author: sereshotes
 */

#include "log_collector.h"

#include "mavlink_help2.h"
#include "router.h"
#include "init_helper.h"
#include "shift_reg.h"


#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#include "esp_log.h"

const static char *TAG = "log";

static log_collector_t _coll;

extern shift_reg_handler_t hsr;

static void log_collector_task(void *arg) {
	log_collector_t *coll = (log_collector_t *)arg;
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();
	while (1) {
		ESP_LOGV(TAG, "log_collector_task");
		mavlink_bcu_stats_t mbs = {0};
		mavlink_message_t msg = {0};
		struct timeval tv = {0};

		xSemaphoreTake(coll->mutex, portMAX_DELAY);
		ESP_LOGV(TAG, "1");
		gettimeofday(&tv, 0);
		uint32_t now = esp_timer_get_time() / 1000;
		coll->conn_stats.time_s = tv.tv_sec;
		coll->conn_stats.time_us = tv.tv_usec;
		coll->conn_stats.time_steady = now;

		mbs.time_s = tv.tv_sec;
		mbs.time_us = tv.tv_usec;
		mbs.time_steady = now;

		mbs.sd_elapsed_time_from_msg 			= coll->log_data[LOG_COMP_ID_SD].ellapsed_time;
		mbs.sd_error_count 						= coll->log_data[LOG_COMP_ID_SD].error_count;
		mbs.sd_last_error 						= coll->log_data[LOG_COMP_ID_SD].last_error;
		mbs.sd_last_state 						= coll->log_data[LOG_COMP_ID_SD].last_state;

		mbs.sins_comm_elapsed_time_from_msg 	= coll->log_data[LOG_COMP_ID_SINC_COMM].ellapsed_time;
		mbs.sins_comm_error_count 				= coll->log_data[LOG_COMP_ID_SINC_COMM].error_count;
		mbs.sins_comm_last_error 				= coll->log_data[LOG_COMP_ID_SINC_COMM].last_error;
		mbs.sins_comm_last_state 				= coll->log_data[LOG_COMP_ID_SINC_COMM].last_state;

		ESP_LOGV(TAG, "2");
		if (hsr.mutex) {
			ESP_LOGV(TAG, "3");
			if (xSemaphoreTake(hsr.mutex, 0) == pdTRUE) {
				mbs.shift_reg_last_state = *((uint32_t *)hsr.byte_arr);
				xSemaphoreGive(hsr.mutex);
			}
		}

		ESP_LOGV(TAG, "4");
		mbs.shift_reg_elapsed_time_from_msg 	= coll->log_data[LOG_COMP_ID_SHIFT_REG].ellapsed_time;
		mbs.shift_reg_error_count 				= coll->log_data[LOG_COMP_ID_SHIFT_REG].error_count;
		mbs.shift_reg_last_error 				= coll->log_data[LOG_COMP_ID_SHIFT_REG].last_error;



		its_rt_sender_ctx_t ctx = {0};
		ctx.from_isr = 0;

		ESP_LOGV(TAG, "5");
		mavlink_msg_bcu_radio_conn_stats_encode(mavlink_system, COMP_ANY_0, &msg, &coll->conn_stats);
		its_rt_route(&ctx, &msg, 0);

		ESP_LOGV(TAG, "6");
		mavlink_msg_bcu_time_sync_stats_encode(mavlink_system, COMP_ANY_0, &msg, &coll->time_stats);
		its_rt_route(&ctx, &msg, 0);

		xSemaphoreGive(coll->mutex);

		ESP_LOGV(TAG, "7");
		mavlink_msg_bcu_stats_encode(mavlink_system, COMP_ANY_0, &msg, &mbs);
		its_rt_route(&ctx, &msg, 0);


		ESP_LOGV(TAG, "8");

		vTaskDelayUntil(&xLastWakeTime, LOG_COLLECTOR_SEND_PERIOD / portTICK_PERIOD_MS);
	}
}

void log_collector_init(log_collector_t * coll) {
	if (!coll) {
		coll = &_coll;
	}
	memset(coll, 0, sizeof(*coll));
	coll->mutex = xSemaphoreCreateMutex();
	xTaskCreate(log_collector_task, "Log collector", configMINIMAL_STACK_SIZE + 2500, coll, 2, 0);
}

/*void log_collector_add_to(log_collector_t *hlc, log_comp_id_t id, const log_data_t *data) {
	ESP_LOGV(TAG, "log_collector_add_to id: %d", id);
	xSemaphoreTake(hlc->mutex, portMAX_DELAY);
	hlc->log_data[id] = *data;
	xSemaphoreGive(hlc->mutex);
}*/

void log_collector_add(log_comp_id_t id, const log_data_t *data) {
	ESP_LOGV(TAG, "log_collector_add id: %d", id);
	if (_coll.mutex && xSemaphoreTake(_coll.mutex, portMAX_DELAY) == pdTRUE) {
		_coll.log_data[id] = *data;
		xSemaphoreGive(_coll.mutex);
	}
}
/*
void log_collector_log_task(log_data_t *data) {
	while (1) {
		if (data && data->last_state == LOG_STATE_OFF) {
			vTaskDelete(0);
		}
		log_collector_add(LOG_COMP_ID_SHIFT_REG, data);
		vTaskDelay(LOG_COLLECTOR_ADD_PERIOD_COMMON / portTICK_PERIOD_MS);
	}
}*/

BaseType_t log_collector_take(uint32_t tickTimeout) {
	if (_coll.mutex) {
		return xSemaphoreTake(_coll.mutex, tickTimeout);
	} else {
		return pdFALSE;
	}
}
BaseType_t log_collector_give() {
	if (_coll.mutex) {
		return xSemaphoreGive(_coll.mutex);
	} else {
		return pdFALSE;
	}
}


mavlink_bcu_radio_conn_stats_t *log_collector_get_conn_stats() {
	ESP_LOGV(TAG, "log_collector_get_conn_stats");
	return &_coll.conn_stats;
}
mavlink_bcu_time_sync_stats_t *log_collector_get_time_sync() {
	ESP_LOGV(TAG, "log_collector_get_time_stats");
	return &_coll.time_stats;
}
