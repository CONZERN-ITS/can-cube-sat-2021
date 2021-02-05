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
	if (rc != 0)
	{
		printf("server init failed: %d\n", rc);
		return EXIT_FAILURE;
	}

	printf("server initialization complete\n");
	server_run(&server);

	printf("server loop ended\n");
	server_deinit(&server);

	printf("server destroyed\n");
	return EXIT_SUCCESS;
}
