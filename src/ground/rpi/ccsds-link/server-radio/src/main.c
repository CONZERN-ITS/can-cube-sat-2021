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
	log_set_level(LOG_TRACE);

	server_config_t config;
	rc = server_config_load(&config);
	if (0 != rc)
	{
		log_fatal("unable to load server config: %d", rc);
		return EXIT_FAILURE;
	}

	rc = server_task(&server, &config);
	if (rc != 0)
	{
		log_fatal("server init failed: %d", rc);
		return EXIT_FAILURE;
	}

	log_info("server destroyed");
	return EXIT_SUCCESS;
}
