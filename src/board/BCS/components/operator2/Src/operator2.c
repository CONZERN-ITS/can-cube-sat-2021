/*
 * operator.c
 *
 *  Created on: Aug 22, 2020
 *      Author: sereshotes
 */

#include <assert.h>
#include <string.h>

#include "operator2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "router.h"
#include "log_collector.h"



static char *TAG = "OP2";


typedef struct {
	uint16_t count_recieved_cmds;
	uint16_t count_executed_cmds;
	uint16_t count_errors;
} op_stats_t;

static void op_task(void *arg) {

	op_stats_t stats = {0};
	mavlink_message_t msg = {0};

	its_rt_task_identifier tid = {
			.name = "op2"
	};
	tid.queue = xQueueCreate(10, MAVLINK_MAX_PACKET_LEN);
	its_rt_register(MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND, tid);
	while (1) {
		if (xQueueReceive(tid.queue, &msg, 500 / portTICK_RATE_MS) == pdTRUE) {
			if (msg.msgid == MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND) {
				mavlink_roze_activate_command_t mrac = {0};
				mavlink_msg_roze_activate_command_decode(&msg, &mrac);
				ESP_LOGE(TAG, "%d:%06d: HAHA for BSK%d for %d ms", (int)mrac.time_s, mrac.time_us, mrac.area_id, mrac.active_time);
				stats.count_recieved_cmds++;
			}
		}
		if (log_collector_take(50 / portTICK_RATE_MS) == pdTRUE) {
			mavlink_bcu_radio_conn_stats_t *conn_stats = log_collector_get_conn_stats();
			conn_stats->count_operator_cmds_executed = stats.count_executed_cmds;
			conn_stats->count_operator_cmds_recieved = stats.count_recieved_cmds;
			conn_stats->count_operator_errors = stats.count_errors;
			conn_stats->update_time_operator = esp_timer_get_time() / 1000;
			log_collector_give();
		}
	}
}
void op2_init() {
	xTaskCreatePinnedToCore(op_task, "OP2 task", configMINIMAL_STACK_SIZE + 3000, 0, 4, 0, tskNO_AFFINITY);
}
