/*
 * mav_packers.c
 *
 *  Created on: 22 апр. 2020 г.
 *      Author: developer
 */

#include <stdint.h>

#include <analog/analog.h>
#include "mav_packet.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <its/mavlink.h>
#pragma GCC diagnostic pop

#include "commissar.h"

#include "drivers/time_svc/time_svc.h"
#include "its-i2c-link.h"
#include "drivers/uplink.h"
#include "state.h"



#define SYSTEM_ID CUBE_1_SINS
#define COMPONENT_ID COMP_ANY_0

int mavlink_sins_isc(stateSINS_isc_t * state_isc)
{

	//printf("mavlink_sins_isc?\n");
	mavlink_sins_isc_t msg_sins_isc;
	msg_sins_isc.time_s = state_isc->tv.tv_sec;
	msg_sins_isc.time_us = state_isc->tv.tv_usec;
	msg_sins_isc.time_steady = HAL_GetTick();


	//printf("Accel: %f %f %f \n", state_isc->accel[0], state_isc->accel[1], state_isc->accel[2]);
	//printf("Mag: %f %f %f \n", state_isc->magn[0], state_isc->magn[1], state_isc->magn[2]);

	for (int i = 0; i < 3; i++)
	{
		msg_sins_isc.accel[i] = state_isc->accel[i];
		msg_sins_isc.compass[i] = state_isc->magn[i];
		msg_sins_isc.quaternion[i] = state_isc->quaternion[i];
	}
	msg_sins_isc.quaternion[3] = state_isc->quaternion[3];


	mavlink_message_t msg;
	mavlink_msg_sins_isc_encode(SYSTEM_ID, COMPONENT_ID, &msg, &msg_sins_isc);
	int error = uplink_write_mav(&msg);
	return error;
}

int mavlink_lds_dir(stateSINS_lds_t * state)
{
	mavlink_lds_dir_t mld;
	mld.time_s = state->tv.tv_sec;
	mld.time_us = state->tv.tv_usec;
	mld.time_steady = HAL_GetTick();
	//printf("Accel: %f %f %f \n", state_lds->accel[0], state_lds->accel[1], state_lds->accel[2]);
	//printf("Mag: %f %f %f \n", state_lds->magn[0], state_lds->magn[1], state_lds->magn[2]);


	mld.x = state->dir[0];
	mld.y = state->dir[1];
	mld.z = state->dir[2];
	mld.error = state->error;


	mavlink_message_t msg;
	mavlink_msg_lds_dir_encode(SYSTEM_ID, COMPONENT_ID, &msg, &mld);
	int error = uplink_write_mav(&msg);
	return error;
}

int mavlink_light_diode(stateSINS_lds_t * state)
{
	mavlink_light_diode_t mld;
	mld.time_s = state->tv.tv_sec;
	mld.time_us = state->tv.tv_usec;
	mld.time_steady = HAL_GetTick();
	//printf("Accel: %f %f %f \n", state_lds->accel[0], state_lds->accel[1], state_lds->accel[2]);
	//printf("Mag: %f %f %f \n", state_lds->magn[0], state_lds->magn[1], state_lds->magn[2]);


	for (int i = 0; i < ITS_SINS_LDS_COUNT; i++) {
		mld.value[i] = state->sensor[i];
	}

	mavlink_message_t msg;
	mavlink_msg_light_diode_encode(SYSTEM_ID, COMPONENT_ID, &msg, &mld);
	int error = uplink_write_mav(&msg);
	return error;
}

int mavlink_timestamp(void)
{
	mavlink_timestamp_t msg_timestamp;
	struct timeval tv;

	time_svc_world_get_time(&tv);
	// Отправляем только если мы находимся ближе к середине секунды
	if (tv.tv_usec <= 100*1000 || tv.tv_usec >= 900*1000)
		return 0;

	msg_timestamp.time_s = tv.tv_sec;
	msg_timestamp.time_us = tv.tv_usec;
	msg_timestamp.time_steady = HAL_GetTick();
	msg_timestamp.time_base = (uint8_t)time_svc_timebase();

	mavlink_message_t msg;
	mavlink_msg_timestamp_encode(SYSTEM_ID, COMPONENT_ID, &msg, &msg_timestamp);
	int error = uplink_write_mav(&msg);
	return error;
}


void on_gps_packet(void * arg, const ubx_any_packet_t * packet)
{
	volatile ubx_pid_t pid = packet->pid;
	switch (pid)
	{
	case UBX_PID_NAV_SOL:
		{
			struct timeval tv;
			time_svc_world_get_time(&tv);

			mavlink_gps_ubx_nav_sol_t msg_gps_ubx_nav_sol;
			msg_gps_ubx_nav_sol.time_s = tv.tv_sec;
			msg_gps_ubx_nav_sol.time_us = tv.tv_usec;
			msg_gps_ubx_nav_sol.time_steady = HAL_GetTick();
			msg_gps_ubx_nav_sol.iTOW = packet->packet.navsol.i_tow;
			msg_gps_ubx_nav_sol.fTOW = packet->packet.navsol.f_tow;
			msg_gps_ubx_nav_sol.week = packet->packet.navsol.week;
			msg_gps_ubx_nav_sol.gpsFix = packet->packet.navsol.gps_fix;
			msg_gps_ubx_nav_sol.flags = packet->packet.navsol.flags;
			msg_gps_ubx_nav_sol.ecefX = packet->packet.navsol.pos_ecef[0];
			msg_gps_ubx_nav_sol.ecefY = packet->packet.navsol.pos_ecef[1];
			msg_gps_ubx_nav_sol.ecefZ = packet->packet.navsol.pos_ecef[2];
			msg_gps_ubx_nav_sol.pAcc = packet->packet.navsol.p_acc;
			msg_gps_ubx_nav_sol.ecefVX = packet->packet.navsol.vel_ecef[0];
			msg_gps_ubx_nav_sol.ecefVY = packet->packet.navsol.vel_ecef[1];
			msg_gps_ubx_nav_sol.ecefVZ = packet->packet.navsol.vel_ecef[2];
			msg_gps_ubx_nav_sol.sAcc = packet->packet.navsol.s_acc;
			msg_gps_ubx_nav_sol.pDOP = packet->packet.navsol.p_dop;
			msg_gps_ubx_nav_sol.numSV = packet->packet.navsol.num_sv;

			mavlink_message_t msg;
			mavlink_msg_gps_ubx_nav_sol_encode(SYSTEM_ID, COMPONENT_ID, &msg, &msg_gps_ubx_nav_sol);
			uplink_write_mav(&msg);
			break;
		}

	default:
		break;

	}
}


int own_temp_packet(void)
{
	int error = 0;

	const int oversampling = 50;
	uint16_t raw;
	error = analog_get_raw(ANALOG_TARGET_INTEGRATED_TEMP, oversampling, &raw);
	if (0 != error)
		return error;

	// Пересчитываем по зашитым калибровочным коэффициентам
	const uint16_t val_c1 = *TEMPSENSOR_CAL1_ADDR;
	const uint16_t val_c2 = *TEMPSENSOR_CAL2_ADDR;
	const uint16_t t_c1 = TEMPSENSOR_CAL1_TEMP;
	const uint16_t t_c2 = TEMPSENSOR_CAL2_TEMP;

	const float slope = (float)(t_c2 - t_c1) / (val_c2 - val_c1);
	float temp = slope * (raw - val_c1) + t_c1;

	// Теперь посчитаем VBAT
	error = analog_get_raw(ANALOG_TARGET_VBAT, oversampling, &raw);
	if (0 != error)
		return error;

	float vbat = (raw * 2) * 3.3f / 0x0FFF; // * 2 потому что  VAT подключен к АЦП с делителем

	// Теперь vdda
	error = analog_get_vdda_mv(oversampling, &raw);
	if (0 != error)
		return error;

	float vdda = (float)raw / 1000;

	struct timeval tv;
	time_svc_world_get_time(&tv);

	mavlink_own_temp_t msg;
	msg.time_s = tv.tv_sec;
	msg.time_us = tv.tv_usec;
	msg.time_steady = HAL_GetTick();
	msg.deg = temp;
	msg.vbat = vbat;
	msg.vdda = vdda;

	mavlink_message_t generic_msg;
	mavlink_msg_own_temp_encode(SYSTEM_ID, COMPONENT_ID, &generic_msg, &msg);
	uplink_write_mav(&generic_msg);

	return 0;
}


int mavlink_errors_packet(void)
{

	struct timeval tv;
	time_svc_world_get_time(&tv);

	mavlink_sins_errors_t errors_msg;

	errors_msg.time_s = tv.tv_sec;
	errors_msg.time_us = tv.tv_usec;
	errors_msg.time_steady = HAL_GetTick();

	errors_msg.analog_sensor_init_error = error_system.analog_sensor_init_error;

	errors_msg.gps_init_error = error_system.gps_init_error;
	errors_msg.gps_config_error = error_system.gps_config_error;
	errors_msg.gps_uart_init_error = error_system.gps_uart_error;
	errors_msg.gps_reconfig_counter = error_system.gps_reconfig_counter;

	errors_msg.mems_i2c_error = error_system.mems_i2c_error;
	errors_msg.mems_i2c_error_counter = error_system.mems_i2c_error_counter;

	errors_msg.lsm6ds3_error = error_system.lsm6ds3_error;
	errors_msg.lsm6ds3_error_counter = error_system.lsm6ds3_error_counter;

	errors_msg.lis3mdl_error = error_system.lis3mdl_error;
	errors_msg.lis3mdl_error_counter = error_system.lis3mdl_error_counter;

	errors_msg.rtc_error = error_system.rtc_error;

	errors_msg.timers_error = error_system.timers_error;

	errors_msg.osc_source = error_system.osc_source;

	errors_msg.reset_counter = error_system.reset_counter;
	errors_msg.reset_cause = error_system.reset_cause;

	mavlink_message_t msg;
	mavlink_msg_sins_errors_encode(SYSTEM_ID, COMPONENT_ID, &msg, &errors_msg);
	uplink_write_mav(&msg);

	return 0;
}


int mavlink_its_link_stats(void)
{
	struct timeval tv;
	time_svc_world_get_time(&tv);

	mavlink_i2c_link_stats_t msg;
	msg.time_s = tv.tv_sec;
	msg.time_us = tv.tv_usec;
	msg.time_steady = HAL_GetTick();

	its_i2c_link_stats_t stats;
	its_i2c_link_stats(&stats);

	msg.rx_packet_start_cnt = stats.rx_packet_start_cnt;
	msg.rx_packet_done_cnt = stats.rx_packet_done_cnt;
	msg.rx_cmds_start_cnt = stats.rx_cmds_start_cnt;
	msg.rx_cmds_done_cnt = stats.rx_cmds_done_cnt;
	msg.rx_drops_start_cnt = stats.rx_drops_start_cnt;
	msg.rx_drops_done_cnt = stats.rx_drops_done_cnt;
	msg.tx_psize_start_cnt = stats.tx_psize_start_cnt;
	msg.tx_psize_done_cnt = stats.tx_psize_done_cnt;
	msg.tx_packet_start_cnt = stats.tx_packet_start_cnt;
	msg.tx_packet_done_cnt = stats.tx_packet_done_cnt;
	msg.tx_zeroes_start_cnt = stats.tx_zeroes_start_cnt;
	msg.tx_zeroes_done_cnt = stats.tx_zeroes_done_cnt;
	msg.tx_empty_buffer_cnt = stats.tx_empty_buffer_cnt;
	msg.tx_overruns_cnt = stats.tx_overruns_cnt;
	msg.cmds_get_size_cnt = stats.cmds_get_size_cnt;
	msg.cmds_get_packet_cnt = stats.cmds_get_packet_cnt;
	msg.cmds_set_packet_cnt = stats.cmds_set_packet_cnt;
	msg.cmds_invalid_cnt = stats.cmds_invalid_cnt;
	msg.restarts_cnt = stats.restarts_cnt;
	msg.berr_cnt = stats.berr_cnt;
	msg.arlo_cnt = stats.arlo_cnt;
	msg.ovf_cnt = stats.ovf_cnt;
	msg.af_cnt = stats.af_cnt;
	msg.btf_cnt = stats.btf_cnt;
	msg.tx_wrong_size_cnt = stats.tx_wrong_size_cnt;
	msg.rx_wrong_size_cnt = stats.rx_wrong_size_cnt;

	mavlink_message_t generic_msg;
	mavlink_msg_i2c_link_stats_encode(SYSTEM_ID, COMPONENT_ID, &generic_msg, &msg);
	uplink_write_mav(&generic_msg);

	return 0;
}


int mavlink_comissar_report()
{
	mavlink_commissar_report_t report;
	uint8_t component_id;
	commissar_provide_report(&component_id, &report);

	mavlink_message_t msg;
	mavlink_msg_commissar_report_encode(SYSTEM_ID, component_id, &msg, &report);
	int error = uplink_write_mav(&msg);
	return error;
}
