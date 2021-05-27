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
		memset(data, 0x00, sizeof(data));
		int rc = its_i2c_link_write(data, sizeof(data));
		printf("%u write rc = %d\n", counter, rc);

		rc = its_i2c_link_read(data, sizeof(data));
		printf("%u read rc = %d\n", counter, rc);

		counter++;
		HAL_Delay(1000);
	}

	return 0;
}
