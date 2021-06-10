#ifndef RADIO_SX126X_EVENTS_H_
#define RADIO_SX126X_EVENTS_H_

#include <stdint.h>
#include <stdbool.h>


typedef enum sx126x_evt_kind_t
{
	SX126X_EVTKIND_NONE = 0x00,
	SX126X_EVTKIND_TX_DONE,
	SX126X_EVTKIND_RX_DONE
} sx126x_evt_kind_t;


typedef union sx126x_evt_arg_t
{
	struct
	{
		bool timed_out;
		bool crc_valid;
	} rx_done;

	struct
	{
		bool timed_out;
	} tx_done;

} sx126x_evt_arg_t;


#endif /* RADIO_SX126X_EVENTS_H_ */
