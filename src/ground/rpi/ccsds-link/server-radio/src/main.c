#include <unistd.h>
#include <signal.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <log.h>

#include "server.h"


static server_t server;


static void signal_handler(int signum)
{
	server_request_stop(&server);
}


int main(void)
{
	int exit_code = EXIT_SUCCESS;
	int rc;
	log_set_level(LOG_INFO);

	server_config_t config;
	rc = server_config_load(&config);
	if (0 != rc)
	{
		log_fatal("unable to load server config: %d", rc);
		return EXIT_FAILURE;
	}

	rc = server_ctor(&server, &config);
	if (0 != rc)
	{
		log_fatal("server ctor failed: %d", rc);
		return EXIT_FAILURE;
	}

	struct sigaction custom_handler = {
			.sa_handler = signal_handler,
			.sa_flags = SA_RESETHAND,
	};

	rc = sigaction(SIGINT, &custom_handler, NULL);
	if (0 != rc)
	{
		log_fatal("unable to install SIGINT handler");
		exit_code = EXIT_FAILURE;
		goto exit;
	}

	rc = sigaction(SIGTERM, &custom_handler, NULL);
	if (0 != rc)
	{
		log_fatal("unable to install SIGTERM handler");
		exit_code = EXIT_FAILURE;
		goto exit;
	}

	rc = server_serve(&server);
	if (0 != rc)
	{
		log_fatal("server_serve returned an error: %d", rc);
	}


exit:
	server_dtor(&server);
	log_info("server destroyed");
	log_info("clean exit");
	return exit_code;
}
