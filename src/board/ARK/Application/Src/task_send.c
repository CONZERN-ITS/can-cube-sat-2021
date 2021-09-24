/*
 * task_send.c
 *
 *  Created on: Apr 25, 2020
 *      Author: sereshotes
 */

#include "main.h"

#include "mavlink_help2.h"
#include "its-time.h"
#include "uplink.h"
#include "task_ds.h"
#include "task_ina.h"
#include "stdlib.h"
#include "inttypes.h"

#include "its-i2c-link.h"


static int tsend_therm_period = 1000 / TDS_TEMP_MAX_COUNT;
static int tsend_elect_period = 2000 / (TINA_COUNT + 1);

static int ds_updated;
static int ina_updated;


typedef enum {
    STATE_WAIT,
    STATE_SENDING,
} send_state_t;

static send_state_t estate;
static send_state_t tstate;

static its_time_t ds_read_time;
static its_time_t ina_read_time;


void ds_callback(void) {
    ds_updated = 1;
    its_gettimeofday(&ds_read_time);
}
void ina_callback(void) {
    ina_updated = 1;
    its_gettimeofday(&ina_read_time);
}

void task_send_init(void *arg) {
    tds18b20_add_callback(ds_callback);
    tina219_add_ina_callback(ina_callback);
}


void eupdate() {
    static float current[TINA_COUNT];
    static float voltage[TINA_COUNT];
    static uint32_t prev = 0;
    static its_time_t tim;
    uint32_t now = HAL_GetTick();
    if (estate == STATE_WAIT && now - prev >= tsend_elect_period) {
        printf("INFO: send ina\n");
        prev = now;

        tina_value_t *tv;
        int *is_valid;
        int count = tina219_get_value(&tv, &is_valid);
        for (int i = 0; i < count; i++) {
            if (is_valid[i]) {
                current[i] = tv[i].current;
                voltage[i] = tv[i].voltage;
            } else {
                current[i] = NAN;
                voltage[i] = NAN;
            }
        }
        estate = STATE_SENDING;
        ina_updated = 0;
        tim = ina_read_time;

    }
    if (estate == STATE_SENDING) {
        static int is_gened = 0;
        static int ina_index = 0;
        static mavlink_message_t msg;
        if (!is_gened) {
            mavlink_electrical_state_t mest = {0};
            mest.time_s = tim.sec;
            mest.time_us = tim.msec * 1000;
            mest.time_steady = HAL_GetTick();

            mest.current = current[ina_index];
            mest.voltage = voltage[ina_index];

            mavlink_msg_electrical_state_encode(mavlink_system, ina_index, &msg, &mest);
        }
        if (uplink_packet(&msg) > 0) {
            ina_index++;
            is_gened = 0;
            if (ina_index >= TINA_COUNT) {
                estate = STATE_WAIT;
                ina_index = 0;
            }

        }
    }
}

void tupdate() {
    static float temp[TDS_TEMP_MAX_COUNT];
    static its_time_t tim;
    static uint32_t prev = 0;
    static int ds_count = 0;
    if (ds_count != 0) {
        tsend_therm_period = 1000 / ds_count;
    }
    if (tstate == STATE_WAIT && ds_updated && HAL_GetTick() - prev >= tsend_therm_period) {
        printf("INFO: send ds\n");

        its_time_t timm;
        its_gettimeofday(&timm);
        prev = HAL_GetTick();
        tstate = STATE_SENDING;
        ds_updated = 0;

        float *arr;
        int *is_valid;
        ds_count = tds18b20_get_value(&arr, &is_valid);
        for (int i = 0; i < ds_count; i++) {
            if (is_valid[i]) {
                temp[i] = arr[i];
            } else {
                temp[i] = NAN;
            }
        }

        tim = ds_read_time;
    }
    if (tstate == STATE_SENDING) {
        static int is_gened = 0;
        static mavlink_message_t msg;
        static int ds_index = 0;
        if (!is_gened) {
            mavlink_thermal_state_t mtst = {0};
            mtst.time_s = tim.sec;
            mtst.time_us = tim.msec * 1000;
            mtst.time_steady = HAL_GetTick();
            mtst.temperature = temp[ds_index];

            mavlink_msg_thermal_state_encode(mavlink_system, ds_index, &msg, &mtst);
        }
        if (uplink_packet(&msg) > 0) {
            ds_index++;
            is_gened = 0;
            if (ds_index >= ds_count) {
                tstate = STATE_WAIT;
                ds_index = 0;
            }

        }
    }
}


void send_stats(void)
{
	static uint32_t prev = 0;
	uint32_t delta_time = 1000;


	if (HAL_GetTick() - prev < delta_time)
		return;


	prev = HAL_GetTick();

	mavlink_message_t msg;
	mavlink_ark_stats_t stats_msg;
	its_collect_ark_stats(&stats_msg);
	mavlink_msg_ark_stats_encode(mavlink_system, (uint8_t)0, &msg, &stats_msg);
	uplink_packet(&msg);

	its_i2c_link_stats_t i2c_stats;
	its_i2c_link_stats(&i2c_stats);

	its_time_t now;
	its_gettimeofday(&now);

	mavlink_i2c_link_stats_t i2c_stats_msg;
	i2c_stats_msg.time_s = now.sec;
	i2c_stats_msg.time_us = now.msec * 1000;
	i2c_stats_msg.time_steady = HAL_GetTick();

	i2c_stats_msg.rx_packet_start_cnt = i2c_stats.rx_packet_start_cnt;
	i2c_stats_msg.rx_packet_done_cnt = i2c_stats.rx_packet_done_cnt;
	i2c_stats_msg.rx_cmds_start_cnt = i2c_stats.rx_cmds_start_cnt;
	i2c_stats_msg.rx_cmds_done_cnt = i2c_stats.rx_cmds_done_cnt;
	i2c_stats_msg.rx_drops_start_cnt = i2c_stats.rx_drops_start_cnt;
	i2c_stats_msg.rx_drops_done_cnt = i2c_stats.rx_drops_done_cnt;

	i2c_stats_msg.tx_psize_start_cnt = i2c_stats.tx_psize_start_cnt;
	i2c_stats_msg.tx_psize_done_cnt = i2c_stats.tx_psize_done_cnt;
	i2c_stats_msg.tx_packet_start_cnt = i2c_stats.tx_packet_start_cnt;
	i2c_stats_msg.tx_packet_done_cnt = i2c_stats.tx_packet_done_cnt;
	i2c_stats_msg.tx_zeroes_start_cnt = i2c_stats.tx_zeroes_start_cnt;
	i2c_stats_msg.tx_zeroes_done_cnt = i2c_stats.tx_zeroes_done_cnt;
	i2c_stats_msg.tx_empty_buffer_cnt = i2c_stats.tx_empty_buffer_cnt;
	i2c_stats_msg.tx_overruns_cnt = i2c_stats.tx_overruns_cnt;

	i2c_stats_msg.cmds_get_size_cnt = i2c_stats.cmds_get_size_cnt;
	i2c_stats_msg.cmds_get_packet_cnt = i2c_stats.cmds_get_packet_cnt;
	i2c_stats_msg.cmds_set_packet_cnt = i2c_stats.cmds_set_packet_cnt;
	i2c_stats_msg.cmds_invalid_cnt = i2c_stats.cmds_invalid_cnt;

	i2c_stats_msg.restarts_cnt = i2c_stats.restarts_cnt;
	i2c_stats_msg.berr_cnt = i2c_stats.berr_cnt;
	i2c_stats_msg.arlo_cnt = i2c_stats.arlo_cnt;
	i2c_stats_msg.ovf_cnt = i2c_stats.ovf_cnt;
	i2c_stats_msg.af_cnt = i2c_stats.af_cnt;
	i2c_stats_msg.btf_cnt = i2c_stats.btf_cnt;
	i2c_stats_msg.tx_wrong_size_cnt = i2c_stats.tx_wrong_size_cnt;
	i2c_stats_msg.rx_wrong_size_cnt = i2c_stats.rx_wrong_size_cnt;

	mavlink_msg_i2c_link_stats_encode(mavlink_system, (uint8_t)0, &msg, &i2c_stats_msg);
	uplink_packet(&msg);
}


void task_send_update(void *arg) {
    static uint32_t prev = 0;
    if (HAL_GetTick() - prev >= 1000){
        prev = HAL_GetTick();
        static its_time_t tim;
        its_gettimeofday(&tim);
        printf("TIME: %"PRIu32".%03"PRIu32"\n", (uint32_t)tim.sec, (uint32_t)tim.msec);
    }
    eupdate();
    tupdate();
    send_stats();

}

