/*
 * main.c
 *
 *  Created on: May 11, 2021
 *      Author: snork
 */

#include <usbd_cdc_if.h>


void accept_cdc_message(void * user_arg, uint8_t * data, size_t data_size)
{
	(void)user_arg;
	CDC_Transmit_FS((uint8_t*)data, data_size);
}


int app_main()
{
	CDC_register_rx_callback(accept_cdc_message, 0);

	while(1)
	{
		//const char message[] = "hello world!\n";
		HAL_Delay(500);
	}

	return 0;
}
