#include "mavlink_main.h"

#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#include <stm32f4xx_hal.h>

#include "its-i2c-link-conf.h"
#include "Inc/its-i2c-link.h"
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
		commissar_report(COMMISSAR_SUB_I2C_LINK, 0);
	}
	else
	{
		commissar_report(COMMISSAR_SUB_I2C_LINK, rc);
	}
}


int mav_main_get_packet_from_its_link(mavlink_message_t * msg)
{
	uint8_t in_buffer[I2C_LINK_PACKET_SIZE];
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
	printf("int ms5611: t=%fC, p=%fpa\n",
			(float)msg->temperature, (float)msg->pressure
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
#ifdef PROCESS_TO_PRINTF
	printf("otemp: %fC\n",
			msg->deg
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);

	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif

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
//	printf("bme-> le: %"PRIi32", ec: %"PRIu16":\n",
//			 msg->int_bme_last_error, msg->int_bme_error_counter
//	);
//
//	printf("adc-> ie: %"PRIi32", le: %"PRIi32", ec: %"PRIu16":\n",
//			msg->adc_init_error, msg->adc_last_error, msg->adc_error_counter
//	);
//
//	printf("me2o2-> ie: %"PRIi32", le: %"PRIi32", ec: %"PRIu16":\n",
//			msg->me2o2_init_error, msg->me2o2_last_error, msg->me2o2_error_counter
//	);
//
//	printf("mics6814-> ie: %"PRIi32", le: %"PRIi32", ec: %"PRIu16":\n",
//			msg->mics6814_init_error, msg->mics6814_last_error, msg->mics6814_error_counter
//	);
//
//	printf("integrated-> ie: %"PRIi32", le: %"PRIi32", ec: %"PRIu16":\n",
//			msg->integrated_init_error, msg->integrated_last_error, msg->integrated_error_counter
//	);
//
//	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
//			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
//	);
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
	/*
	printf("it2link rx-> done: %"PRIu16", dropped: %"PRIu16", errors: %"PRIu16"\n",
			msg->rx_done_cnt, msg->rx_dropped_cnt, msg->rx_error_cnt
	);

	printf("it2link tx-> done: %"PRIu16", dropped: %"PRIu16", errors: %"PRIu16" ovr: %"PRIu16"\n",
			msg->tx_done_cnt, msg->tx_zeroes_cnt, msg->tx_error_cnt, msg->tx_overrun_cnt
	);

	printf("it2link restarts: %"PRIu16", listen done: %"PRIu16", last error: %"PRIi32"\n",
			msg->restarts_cnt, msg->listen_done_cnt, msg->last_error
	);

	printf("time = 0x%08"PRIX32"%08"PRIX32", %08"PRIX32"\n",
			(uint32_t)(msg->time_s >> 4*8), (uint32_t)(msg->time_s & 0xFFFFFFFF), msg->time_us
	);
	printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	*/
#endif

#ifdef PROCESS_TO_ITSLINK
    mavlink_message_t ms;
    mavlink_msg_i2c_link_stats_encode(mavlink_system, COMP_ANY_0, &ms, msg);
    uint16_t size = mavlink_msg_to_send_buffer(_its_link_output_buf, &ms);
    mav_main_send_to_its_link(MAVLINK_COMM_0, _its_link_output_buf, size);
#endif
}

