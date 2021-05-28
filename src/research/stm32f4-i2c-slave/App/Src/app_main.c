#include <its-i2c-link.h>
#include <string.h>
#include <stdio.h>

#include "main.h"


int app_main()
{
	its_i2c_link_start();

	uint8_t data[200];
	size_t counter = 0;
	while(1)
	{
		memset(data, counter & 0xFF, sizeof(data));
		int rc = its_i2c_link_write(data, sizeof(data));
		//printf("%u write rc = %d\n", counter, rc);

		rc = its_i2c_link_read(data, sizeof(data));
		if (rc > 0)
		{
			printf("%u got packet; rc = %d\n", counter, rc);
			for (int i = 0; i < rc; i++)
				printf("%02X", (int)data[i]);

			printf("\n");
		}

		counter++;
		HAL_Delay(100);
	}

	return 0;
}
