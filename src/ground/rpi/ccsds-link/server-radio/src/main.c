#include <unistd.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sx126x_board_rpi.h>

#include "server.h"


static server_t server;


int main(void)
{
	int rc;

	rc = server_init(&server);
	if (rc <= 0)
		printf("server init failed: %d\n", rc);

	server_run(&server);

	server_deinit(&server);
	return 0;
}
