#include <unistd.h>
#include <poll.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <log.h>

#include <sx126x_board_rpi.h>

#include "server.h"


static server_t server;


int main(void)
{
	int rc;
	log_set_level(LOG_INFO);

	rc = server_init(&server);
	if (rc != 0)
	{
		log_fatal("server init failed: %d", rc);
		return EXIT_FAILURE;
	}

	log_info("server initialization complete");
	server_run(&server);

	log_info("server loop ended");
	server_deinit(&server);

	log_info("server destroyed");
	return EXIT_SUCCESS;
}
