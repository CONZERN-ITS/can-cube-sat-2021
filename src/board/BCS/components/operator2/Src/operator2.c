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



static char *TAG = "OP2";


static void op_task(void *arg) {

	mavlink_message_t msg = {0};

	its_rt_task_identifier tid = {
			.name = "op2"
	};
	tid.queue = xQueueCreate(10, MAVLINK_MAX_PACKET_LEN);
	its_rt_register(MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND, tid);
	while (1) {
		xQueueReceive(tid.queue, &msg, portMAX_DELAY);
		if (msg.msgid == MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND) {
			mavlink_roze_activate_command_t mrac = {0};
			mavlink_msg_roze_activate_command_decode(&msg, &mrac);
			ESP_LOGE(TAG, "%d:%06d: HAHA for BSK%d for %d ms", (int)mrac.time_s, mrac.time_us, mrac.area_id, mrac.active_time);
		}
	}
}
void op2_init() {
	xTaskCreatePinnedToCore(op_task, "OP2 task", configMINIMAL_STACK_SIZE + 3000, 0, 4, 0, tskNO_AFFINITY);
}
