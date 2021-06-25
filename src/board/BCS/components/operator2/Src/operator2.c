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
#include "shift_reg.h"
#include "pinout_cfg.h"


#define ROZE_COUNT 4
#define CMD_COUNT 4

static char *TAG = "OP2";

typedef enum {
	CS_OFF,
	CS_GOT_MSG,
	CS_ON,
} control_state_t;

typedef struct {
	uint16_t count_recieved_cmds;
	uint16_t count_executed_cmds;
	uint16_t count_errors;
	uint8_t last_executed_cmd_seq;
} op_stats_t;

typedef struct {
	int64_t start;
	int64_t end;
	int64_t duration;
	uint32_t state;
	uint8_t seq;
} state_t;
typedef struct {
	state_t roze[ROZE_COUNT];
	state_t cmd[CMD_COUNT];
} op_state_t;


extern shift_reg_handler_t hsr;

const static int pin_roze[] = {
		ITS_PIN_SR_BSK1_ROZE,
		ITS_PIN_SR_BSK2_ROZE,
		ITS_PIN_SR_BSK3_ROZE,
		ITS_PIN_SR_BSK4_ROZE,
};

const static int pin_cmd[] = {
		ITS_PIN_SR_BSK1_CMD,
		ITS_PIN_SR_BSK2_CMD,
		ITS_PIN_SR_BSK3_CMD,
		ITS_PIN_SR_BSK4_CMD,
};

static void op_task(void *arg) {
	op_state_t op_state = {0};
	op_stats_t stats = {0};
	mavlink_message_t msg = {0};

	its_rt_task_identifier tid = {
			.name = "op2"
	};
	tid.queue = xQueueCreate(10, MAVLINK_MAX_PACKET_LEN);
	its_rt_register(MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND, tid);
	its_rt_register(MAVLINK_MSG_ID_CMD_ACTIVATE_COMMAND, tid);
	its_rt_register(MAVLINK_MSG_ID_IDLE_COMMAND, tid);
	while (1) {
		vTaskDelay(1);

		//-------------------------------------------------------------------------------
		// Читаем сообщения
		//-------------------------------------------------------------------------------
		while (xQueueReceive(tid.queue, &msg, 0) == pdTRUE) {
			if (msg.msgid == MAVLINK_MSG_ID_ROZE_ACTIVATE_COMMAND) {
				mavlink_roze_activate_command_t mrac = {0};
				mavlink_msg_roze_activate_command_decode(&msg, &mrac);
				op_state.roze[mrac.area_id - 1].duration = mrac.active_time * 1000000;
				op_state.roze[mrac.area_id - 1].state = CS_GOT_MSG;
				op_state.roze[mrac.area_id - 1].seq = msg.seq;

				ESP_LOGV(TAG, "%d:%06d: roze for BSK%d for %d ms", (int)mrac.time_s, mrac.time_us, mrac.area_id, mrac.active_time);
				stats.count_recieved_cmds++;
			}
			if (msg.msgid == MAVLINK_MSG_ID_CMD_ACTIVATE_COMMAND) {
				mavlink_cmd_activate_command_t mcac = {0};
				mavlink_msg_cmd_activate_command_decode(&msg, &mcac);
				op_state.cmd[mcac.area_id - 1].duration = mcac.active_time * 1000000;
				op_state.cmd[mcac.area_id - 1].state = CS_GOT_MSG;
				op_state.cmd[mcac.area_id - 1].seq = msg.seq;

				ESP_LOGV(TAG, "%d:%06d: cmd for BSK%d for %d ms", (int)mcac.time_s, mcac.time_us, mcac.area_id, mcac.active_time);
				stats.count_recieved_cmds++;
			}
			if (msg.msgid == MAVLINK_MSG_ID_IDLE_COMMAND) {

				mavlink_idle_command_t mic = {0};
				mavlink_msg_idle_command_decode(&msg, &mic);
				ESP_LOGV(TAG, "%d:%06d: idle", (int)mic.time_s, mic.time_us);
				stats.count_recieved_cmds++;
				stats.count_executed_cmds++;
				stats.last_executed_cmd_seq = msg.seq;
			}
		}

		//-------------------------------------------------------------------------------
		// Управление всем
		//-------------------------------------------------------------------------------

		int64_t now = esp_timer_get_time();
		for (int i = 0; i < ROZE_COUNT; i++) {
			if (op_state.roze[i].state == CS_GOT_MSG) {
				op_state.roze[i].start = now;
				op_state.roze[i].end = now + op_state.roze[i].duration;
				op_state.roze[i].state = CS_ON;
				shift_reg_set_level_pin(&hsr, pin_roze[i], 1);
			}
			if (op_state.roze[i].state == CS_ON) {
				if (op_state.roze[i].end <= now) {
					shift_reg_set_level_pin(&hsr, pin_roze[i], 0);
					op_state.roze[i].state = CS_OFF;
					stats.count_executed_cmds++;
					stats.last_executed_cmd_seq = op_state.roze[i].seq;
				}
			}
		}
		for (int i = 0; i < CMD_COUNT; i++) {
			if (op_state.cmd[i].state == CS_GOT_MSG) {
				op_state.cmd[i].start = now;
				op_state.cmd[i].end = now + op_state.cmd[i].duration;
				op_state.cmd[i].state = CS_ON;
				shift_reg_set_level_pin(&hsr, pin_cmd[i], 1);
			}
			if (op_state.cmd[i].state == CS_ON) {
				if (op_state.cmd[i].end <= now) {
					shift_reg_set_level_pin(&hsr, pin_cmd[i], 0);
					op_state.cmd[i].state = CS_OFF;
					stats.count_executed_cmds++;
					stats.last_executed_cmd_seq = op_state.cmd[i].seq;
				}
			}
		}

		shift_reg_load(&hsr);



		//-------------------------------------------------------------------------------
		// Логи
		//-------------------------------------------------------------------------------

		if (log_collector_take(0) == pdTRUE) {
			mavlink_bcu_radio_conn_stats_t *conn_stats = log_collector_get_conn_stats();
			conn_stats->count_operator_cmds_executed = stats.count_executed_cmds;
			conn_stats->count_operator_cmds_recieved = stats.count_recieved_cmds;
			conn_stats->count_operator_errors = stats.count_errors;
			conn_stats->last_executed_cmd_seq = stats.last_executed_cmd_seq;
			conn_stats->update_time_operator = esp_timer_get_time() / 1000;
			log_collector_give();
		}
	}
}
void op2_init() {
	xTaskCreatePinnedToCore(op_task, "OP2 task", configMINIMAL_STACK_SIZE + 3000, 0, 20, 0, tskNO_AFFINITY);
}
