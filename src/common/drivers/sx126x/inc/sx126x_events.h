#ifndef RADIO_SX126X_EVENTS_H_
#define RADIO_SX126X_EVENTS_H_

#include <stdint.h>

#include "sx126x_defs.h"

struct sx126x_drv_t;
typedef struct sx126x_drv_t sx126x_drv_t;

typedef enum sx126x_evt_kind_t
{
	SX126X_EVT_KIND_TX_DONE,
	SX126X_EVT_KIND_RX_DONE,
	SX126X_EVTKIND_CAD_DONE,

	SX126X_EVTKIND_TIMEOUT,

	SX126X_EVTKIND_GOT_PREAMBLE,
	SX126X_EVTKIND_GOT_HEADER,

	SX126X_EVTKIND_RSSI_MEASURED,
} sx126x_evt_kind_t;


typedef union sx126x_evt_arg_t
{
	struct
	{
		bool packet_valid;
		union
		{
			sx126x_lora_packet_status_t lora;
			sx126x_lora_packet_status_t gfsk;
		} packet_status;
	} rx_done;

	struct
	{
		bool cad_detected;
	} cad_done;

	struct
	{
		bool header_valid;
	} got_header;

	struct
	{
		int8_t value;
	} rssi;

} sx126x_evt_arg_t;


typedef void (*sx126x_evt_handler_t)(
		sx126x_drv_t * /* drv*/,
		void * /*user_arg*/,
		sx126x_evt_kind_t /*evt_kind*/,
		const sx126x_evt_arg_t * /* evt_arg */
);


#endif /* RADIO_SX126X_EVENTS_H_ */
