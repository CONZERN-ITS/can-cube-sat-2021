#include "mavlink_main.h"

#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#include <stm32f4xx_hal.h>

#include "its-i2c-link-conf.h"
#include "its-i2c-link.h"
#include "commissar.h"


extern IWDG_HandleTypeDef hiwdg;

uint8_t mavlink_system = CUBE_1_PL;
static uint8_t _its_link_output_buf[MAVLINK_MAX_PACKET_LEN];


// Инфраструктура для работы с сообщениями из отладочного уарта

#define IMITATOR_CTL_INPUT_BUF_SIZE (MAVLINK_MAX_PACKET_LEN*3)

static uint8_t _imitator_ctl_input_buf[IMITATOR_CTL_INPUT_BUF_SIZE];
volatile size_t _imitator_ctl_input_buf_head = 0;
volatile size_t _imitator_ctl_input_buf_tail = 0;


//! Функция для разбра входяшего байтика
/*! Это функции нет в хедере, она сугубо внутренняя
 *  Предполагается, что она будет вызываться в обработчике прерываний отладочного уарта */
void mav_main_imitator_ctl_consume_byte(uint8_t byte)
{
	_imitator_ctl_input_buf[_imitator_ctl_input_buf_head] = byte;
	_imitator_ctl_input_buf_head += 1;

	if (_imitator_ctl_input_buf_head >= IMITATOR_CTL_INPUT_BUF_SIZE)
		_imitator_ctl_input_buf_head = 0;
}


void mav_main_send_to_its_link(mavlink_channel_t channel, const uint8_t * buffer, uint16_t buffer_size)
{
	(void)channel;
	int rc = its_i2c_link_write(buffer, buffer_size);
	if (rc > 0)
	{
		// Сообщение успешно отправлено
		HAL_IWDG_Refresh(&hiwdg);
		// Для комиссара покажем код ошибки 0
		commissar_accept_report(COMMISSAR_SUB_I2C_LINK, 0);
	}
	else
	{
		commissar_accept_report(COMMISSAR_SUB_I2C_LINK, rc);
	}
}


int mav_main_get_packet_from_its_link(mavlink_message_t * msg)
{
	uint8_t in_buffer[I2C_LINK_MAX_PACKET_SIZE];
	int rcved_size = its_i2c_link_read(in_buffer, sizeof(in_buffer));
	if (rcved_size < 0)
		return rcved_size;

	// о, что-то пришло
	// Считаем что у нас не больше одного мав сообщения на один i2c-link пакет
	for (int i = 0; i < rcved_size; i++)
	{
		mavlink_status_t status;
		uint8_t byte = in_buffer[i];
		int parsed = mavlink_parse_char(MAVLINK_COMM_0, byte, msg, &status);
		if (parsed)
			return 0;
	}

	return -EAGAIN;
}


int mav_main_get_packet_from_imitator_ctl(mavlink_message_t * msg)
{
	while(_imitator_ctl_input_buf_tail != _imitator_ctl_input_buf_tail)
	{
		mavlink_status_t status;
		uint8_t byte = _imitator_ctl_input_buf[_imitator_ctl_input_buf_tail];

		int parsed = mavlink_parse_char(MAVLINK_COMM_1, byte, msg, &status);
		if (parsed)
			return 0;
	}

	return -EAGAIN;
}


void mav_main_process_bme_message(const mavlink_pld_bme280_data_t * msg, PLD_LOCATION location)
{
#ifdef PROCESS_TO_PRINTF
    printf("int bme: t=%fC, p=%fpa, hum=%f%%, alt=%fm\n",
            msg->temperature, msg->pressure, msg->humidity, msg->altitude
    );

    printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
            (uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
    );

    printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
    mavlink_message_t ms;
    mavlink_msg_pld_bme280_data_encode(mavlink_system, location, &ms, msg);
    uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
    mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif

}



void mav_main_process_ms5611_message(const mavlink_pld_ms5611_data_t * msg, PLD_LOCATION location)
{
#ifdef PROCESS_TO_PRINTF
	printf("int ms5611: t=%fC, p=%fpa, alt=%f\n",
			(float)msg->temperature, (float)msg->pressure, (float)msg->altitude
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
    mavlink_message_t ms;
    mavlink_msg_pld_ms5611_data_encode(mavlink_system, location, &ms, msg);
    uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
    mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif

}


void mav_main_process_me2o2_message(mavlink_pld_me2o2_data_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("me2o2: o2=%f%%\n",
			msg->o2_conc
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_pld_me2o2_data_encode(mavlink_system, COMP_ANY_0, &ms, msg);
    uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
    mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_mics_message(mavlink_pld_mics_6814_data_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("mics6814: co=%fppm, nh3=%fppm, no2=%fppm\n",
			msg->co_conc, msg->nh3_conc, msg->no2_conc
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_pld_mics_6814_data_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_owntemp_message(mavlink_own_temp_t * msg)
{
//#ifdef PROCESS_TO_PRINTF
	printf("otemp: %fC, vbat: %f mv, vdda: %f mv\n",
			msg->deg, msg->vbat, msg->vdda
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
//#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_own_temp_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_dosim_message(mavlink_pld_dosim_data_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("dosim : count_tick=%lu, delta time=%lu\n",
			msg->count_tick, msg->delta_time
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_pld_dosim_data_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_dna_message(mavlink_pld_dna_data_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("dna: temp=%f, heater is on=%u\n",
			msg->dna_temp, msg->heater_is_on
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_pld_dna_data_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}



void mav_main_process_own_stats(mavlink_pld_stats_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("resets_cnt-> ie: %"PRIu16", reason: %"PRIu16"\n",
			msg->resets_count, msg->reset_cause
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);
	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_pld_stats_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_i2c_link_stats(mavlink_i2c_link_stats_t * msg)
{
#ifdef PROCESS_TO_PRINTF
	printf("its-i2c-link:rx_p:%04"PRIx16"-%04"PRIx16"; ", msg->rx_packet_start_cnt, msg->rx_packet_done_cnt);
	printf("its-i2c-link:rx_c:%04"PRIx16"-%04"PRIx16"; ", msg->rx_cmds_start_cnt, msg->rx_cmds_done_cnt);
	printf("its-i2c-link:rx_d:%04"PRIx16"-%04"PRIx16"\n", msg->rx_drops_start_cnt, msg->rx_drops_done_cnt);

	printf("its-i2c-link:tx_s:%04"PRIx16"-%04"PRIx16"; ", msg->tx_psize_start_cnt, msg->tx_psize_done_cnt);
	printf("its-i2c-link:tx_p:%04"PRIx16"-%04"PRIx16"; ", msg->tx_packet_start_cnt, msg->tx_packet_done_cnt);
	printf("its-i2c-link:tx_z:%04"PRIx16"-%04"PRIx16"; ", msg->tx_zeroes_start_cnt, msg->tx_zeroes_done_cnt);
	printf("its-i2c-link:tx_o:%04"PRIx16"-%04"PRIx16"\n", msg->tx_empty_buffer_cnt, msg->tx_overruns_cnt);

	printf("its-i2c-link:cmds:%04"PRIx16"-%04"PRIx16"-%04"PRIx16"-%04"PRIx16"\n",
			msg->cmds_get_size_cnt, msg->cmds_get_packet_cnt,
			msg->cmds_set_packet_cnt, msg->cmds_invalid_cnt
	);

	printf("its-i2c-link:errr:%04"PRIx16",%04"PRIx16",%04"PRIx16",%04"PRIx16","
			"%04"PRIx16",%04"PRIx16",%04"PRIx16",%04"PRIx16"\n",
			msg->restarts_cnt, msg->berr_cnt, msg->arlo_cnt, msg->ovf_cnt,
			msg->af_cnt, msg->btf_cnt, msg->tx_wrong_size_cnt, msg->rx_wrong_size_cnt
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);
	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_i2c_link_stats_encode(mavlink_system, COMP_ANY_0, &ms, msg);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_commissar_report(uint8_t component_id, const mavlink_commissar_report_t * report)
{
#ifdef PROCESS_TO_PRINTF
	printf("commisar_report for %d\n", (int)component_id);

	printf(
		"last_report: 0x%08"PRIx32", last_good_report: 0x%08"PRIx32", "
		"reports: %"PRId32", bad_reports: %"PRId32"\n",
		report->last_report_time, report->last_good_report_time,
		report->reports_counter, report->bad_reports_counter
	);

	printf(
		"punishments: %"PRIu32", last_one: 0x%08"PRIx32"\n",
		report->punishments_counter, report->last_punishment_time
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
		(uint32_t)(report->time_s >> 4*8), (uint32_t)(report->time_s & 0xFFFFFFFF), report->time_us
	);
	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

#ifdef PROCESS_TO_ITSLINK
	mavlink_message_t ms;
	mavlink_msg_commissar_report_encode(mavlink_system, component_id, &ms, report);
	uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
	mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}


void mav_main_process_ccompressor_state(const mavlink_pld_compressor_data_t * msg)
{
	printf("compressor state\n");

	printf(
		"pump_on: %d, valve_open: %d\n",
		(int)msg->pump_on, (int)msg->valve_open
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
		(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);
	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
}
