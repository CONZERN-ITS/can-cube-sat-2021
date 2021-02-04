#ifndef SERVER_RADIO_SRC_RADIO_CONFIG_H_
#define SERVER_RADIO_SRC_RADIO_CONFIG_H_

#include "sx126x_drv.h"

#define RADIO_PACKET_SIZE 200


typedef struct radio_t
{
	sx126x_drv_t dev;
} radio_t;


int radio_init(radio_t * radio);

void radio_deinit(radio_t * radio);


#endif /* SERVER_RADIO_SRC_RADIO_CONFIG_H_ */
