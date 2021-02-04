#include <unistd.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sx126x_board_rpi.h>

#include "radio.h"

static uint8_t _pbuff[255] = {0};


static void _gen_packet(uint8_t * where, uint8_t where_size)
{
	static uint8_t serial = 0;
	memset(where, serial, where_size);
	serial++;
}


static int _send_packet(sx126x_drv_t * radio, uint8_t psize)
{
	int rc;
	_gen_packet(_pbuff, psize);

	rc = sx126x_drv_payload_write(radio, _pbuff, psize);
	if (0 != rc) return rc;

	rc = sx126x_drv_mode_tx(radio, 0);
	if (0 != rc) return rc;

	printf("sent packet %d\n", (int)_pbuff[0]);
	return rc;
}


static int _fetch_packet(sx126x_drv_t * drv)
{
	int rc;
	uint8_t rx_size;

	rc = sx126x_drv_payload_rx_size(drv, &rx_size);
	if (0 != rc) return rc;

	rc = sx126x_drv_payload_read(drv, _pbuff, rx_size);
	if (0 != rc) return rc;

	printf("got packet %d\n", (int)_pbuff[0]);
	return 0;
}


static void _event_handler(sx126x_drv_t * drv, void * user_arg, sx126x_evt_kind_t kind, const sx126x_evt_arg_t * arg)
{
	int rc;

	switch (kind)
	{
	case SX126X_EVTKIND_RX_DONE:
		if (arg->rx_done.timed_out)
		{
			// Если эфир свободен
			// Отправляем свой пакет
			rc = _send_packet(drv, RADIO_PACKET_SIZE);
			if (0 != rc) break;
		}
		else
		{
			// Если нет - достаем пакет и идем слушать новый
			rc = _fetch_packet(drv);
			if (0 != rc) break;

			rc = sx126x_drv_mode_rx(drv, 0);
			if (0 != rc) break;
		}
		break;

	case SX126X_EVTKIND_TX_DONE:
		if (arg->tx_done.timed_out)
		{
			printf("tx timed out!\n");
		}

		// Начинаем слушать
		rc = sx126x_drv_mode_rx(drv, 0);
		if (0 != rc) break;

		break;

	case SX126X_EVTKIND_CAD_DONE:
		printf("cad done?\n");
		break;

	default:
		printf("unknown event: %d\n", kind);
	};


	if (0 != rc)
	{
		printf("event handler error: %d\n", rc);
	}
}


int main(void)
{
	int rc;
//
//	int event_fd = sx126x_brd_rpi_get_event_fd(_radio.api.board);
//	if (event_fd < 0)
//	{
//		perror("unable to fetch event fd");
//		return EXIT_FAILURE;
//	}
//
//	while(1)
//	{
//		struct pollfd fds[1] = {0};
//		fds[0].fd = event_fd;
//		fds[0].events = POLLIN;
//
//		// Сперва поллим события
//		// Поставим таймаут, чтобы если вдруг что-то пошло не так
//		// мы бы могли всеравно подергать радио
//		rc = poll(fds, sizeof(fds)/sizeof(*fds), 5000);
//		int flush_rc = 0;
//		if (rc < 0)
//		{
//			perror("poll for interrupt line failed");
//			return EXIT_FAILURE;
//		}
//		else if (rc > 0)
//		{
//			// Сливаем евент
//			flush_rc = sx126x_brd_rpi_flush_event(_radio.api.board);
//		}
//		printf("poll rc = %d, flush rc = %d\n", rc, flush_rc);
//
//
//		rc = sx126x_drv_poll(&_radio);
//		if (rc)
//			printf("drv_poll error: %d\n", rc);
//	}


	return 0;
}
