#include <its-i2c-link.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "main.h"


int app_main()
{
	its_i2c_link_start();

	uint8_t data[200];
	size_t counter = 0;
	uint32_t next_report_ts = 0;
	uint32_t report_period = 1000;
	while(1)
	{
		uint32_t now = HAL_GetTick();
		if (now > next_report_ts)
		{
			next_report_ts += report_period;
			its_i2c_link_stats_t stats;
			its_i2c_link_stats(&stats);

			printf("link stats:\n");

			printf("rx_p:%04"PRIx16"-%04"PRIx16"; ", stats.rx_packet_start_cnt, stats.rx_packet_done_cnt);
			printf("rx_c:%04"PRIx16"-%04"PRIx16"; ", stats.rx_cmds_start_cnt, stats.rx_cmds_done_cnt);
			printf("rx_d:%04"PRIx16"-%04"PRIx16"\n", stats.rx_drops_start_cnt, stats.rx_drops_done_cnt);

			printf("ts_s:%04"PRIx16"-%04"PRIx16"; ", stats.tx_psize_start_cnt, stats.tx_psize_done_cnt);
			printf("ts_p:%04"PRIx16"-%04"PRIx16"; ", stats.tx_packet_start_cnt, stats.tx_packet_done_cnt);
			printf("ts_z:%04"PRIx16"-%04"PRIx16"; ", stats.tx_zeroes_start_cnt, stats.tx_zeroes_done_cnt);
			printf("ts_o:%04"PRIx16"-%04"PRIx16"\n", stats.tx_empty_buffer_cnt, stats.tx_overruns_cnt);

			printf("cd_z:%04"PRIx16"-%04"PRIx16"-%04"PRIx16"-%04"PRIx16"\n",
					stats.cmds_get_size_cnt, stats.cmds_get_packet_cnt,
					stats.cmds_set_packet_cnt, stats.cmds_invalid_cnt
			);

			printf("rstr:%04"PRIx16",%04"PRIx16",%04"PRIx16",%04"PRIx16","
					"%04"PRIx16",%04"PRIx16",%04"PRIx16",%04"PRIx16"\n",
					stats.restarts_cnt, stats.berr_cnt, stats.arlo_cnt, stats.ovf_cnt,
					stats.af_cnt, stats.btf_cnt, stats.tx_wrong_size_cnt, stats.rx_wrong_size_cnt
			);
		}

		memset(data, counter & 0xFF, sizeof(data));
		int rc = its_i2c_link_write(data, sizeof(data));
		//printf("%u write rc = %d\n", counter, rc);

		rc = its_i2c_link_read(data, sizeof(data));
		if (rc > 0)
		{
			//printf("%u got packet; rc = %d\n", counter, rc);
			//for (int i = 0; i < rc; i++)
			//	printf("%02X", (int)data[i]);

			//printf("\n");
		}
		counter++;
		HAL_Delay(1);
	}

	return 0;
}
