/*
 * imitators.c
 *
 *  Created on: May 11, 2021
 *      Author: snork
 */


#include <sensors/its_imitators.h>

#include <errno.h>


static mavlink_pld_ms5611_data_t _ext_ms5611_data;
static mavlink_pld_bme280_data_t _ext_bme280_data;


int its_imitators_process_input(void)
{
	mavlink_message_t msg;
	int rc = mav_main_get_packet_from_imitator_ctl(&msg);

	if (0 != rc)
		return rc;

	switch (msg.msgid)
	{
	case MAVLINK_MSG_ID_PLD_MS5611_DATA:
		mavlink_msg_pld_ms5611_data_decode(&msg, &_ext_ms5611_data);
		break;

	case MAVLINK_MSG_ID_PLD_BME280_DATA:
		mavlink_msg_pld_bme280_data_decode(&msg, &_ext_bme280_data);
		break;
	};

	return 0;
}


int its_imitators_bme280_read(its_bme280_id_t id, mavlink_pld_bme280_data_t * data)
{
	switch (id)
	{
	case ITS_BME_LOCATION_EXTERNAL:
		*data = _ext_bme280_data;
		return 0;

	default:
		break;
	}

	return -ENODEV;
}


int int_imitators_ms5611_read_and_calculate(its_ms5611_id_t id, mavlink_pld_ms5611_data_t * data)
{
	switch(id)
	{
	case ITS_MS_EXTERNAL:
		*data = _ext_ms5611_data;
		return 0;

	default:
		break;
	}

	return -ENODEV;
}
