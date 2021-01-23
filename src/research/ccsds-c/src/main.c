/*
 * main.c
 *
 *  Created on: 15 янв. 2021 г.
 *      Author: HP
 */

#include <ccsds/sdl/usdl/usdl.h>
#include <stdio.h>
#include <endian.h>
#include <ccsds/nl/epp/epp.h>

void print_bit(uint8_t num) {
	for(int i = 0; i < 8; ++i) {
		printf("%u", num >> (7 - i) & 1);
	}
}


void print_bit_arr(uint8_t *arr, size_t size) {
	for (int i = 0; i < size; i++) {
		print_bit(arr[i]);
		printf(" ");
	}
}



int main() {
	printf("HELLO\n");
	pc_t pc_s = {0};
	pc_paramaters_t pc_p = {0};
	pc_p.can_generate_oid = 0;
	pc_p.is_fec_presented = 0;
	pc_p.tf_length = 30;
	pc_p.tft = TF_FIXED;
	pc_p.tfvn = 0x0C;
	uint8_t pc_arr[40];
	pc_init(&pc_s, &pc_p, pc_arr, sizeof(pc_arr));

	mc_mx_t mc_mx_s = {0};
	mc_mx_init(&mc_mx_s, &pc_s);

	mc_t mc_s = {0};
	mc_paramaters_t mc_p = {0};
	mc_p.ocf_flag = 0;
	mc_p.scid = 0x1234;
	mc_p.tft = TF_FIXED;
	mc_init(&mc_s, &mc_mx_s, &mc_p, 0);

	vc_mx_t vc_mx_s = {0};
	vc_mx_init(&vc_mx_s, &mc_s);

	vc_t vc_s = {0};
	vc_parameters_t vc_p = {0};
	vc_p.clcw_report_rate = 0;
	vc_p.clcw_version = 0;
	vc_p.cop_in_effect = COP_NONE;
	vc_p.ocf_allowed = 0;
	vc_p.repetition_max_cop = 1;
	vc_p.repetition_max_user = 1;
	vc_p.transfer_frame_type = TF_FIXED;
	vc_p.vc_id = 0;
	vc_p.vcf_count_lenght_expedited = 1;
	vc_p.vcf_count_lenght_seq_control = 1;
	vc_init(&vc_s, &vc_mx_s, &vc_p, 0);

	map_mx_t map_mx_s = {0};
	map_mx_init(&map_mx_s, &vc_s);

	map_t map_s = {0};
	uint8_t map_buffer[40];
	map_s.buf_ex.data = map_buffer;
	map_s.buf_ex.max_size = sizeof(map_buffer);
	mapp_init(&map_s, &map_mx_s, 5);


	uint8_t payload[] = "\xDE\xAD\xBE\xAF";
	epp_header_t epp_header = {0};
	epp_header.epp_id = 1;
	uint8_t pl_arr[40];
	int epp_size = epp_make_header_auto_length2(&epp_header, pl_arr, sizeof(pl_arr), sizeof(payload));
	memcpy(&pl_arr[epp_size], payload, sizeof(payload));
	mapp_send(&map_s, pl_arr, sizeof(payload) + epp_size, PVN_ENCAPSULATION_PROTOCOL, QOS_EXPEDITED);
	pc_request_from_down(&pc_s);

	for (int i = 0; i < pc_p.tf_length; i++) {
		printf("0x%x ", pc_arr[i]);
	}

	printf("\n\nHELLO2\n");

	pc_t pc_r = {0};
	pc_init(&pc_r, &pc_p, 0, 0);

	mc_mx_t mc_mx_r = {0};
	mc_mx_init(&mc_mx_r, &pc_r);

	mc_t mc_r = {0};
	mc_p.scid = 0x1235;
	mc_init(&mc_r, &mc_mx_r, &mc_p, 0);

	vc_mx_t vc_mx_r = {0};
	vc_mx_init(&vc_mx_r, &mc_r);

	vc_t vc_r = {0};
	vc_init(&vc_r, &vc_mx_r, &vc_p, 0);

	map_mx_t map_mx_r = {0};
	map_mx_init(&map_mx_r, &vc_r);

	map_t map_r = {0};
	uint8_t map_buffer1[40];
	uint8_t map_buffer2[40];
	map_r.mapr.packet.data = map_buffer1;
	map_r.mapr.packet.max_size = sizeof(map_buffer1);
	map_r.mapr.tfdz.data = map_buffer2;
	map_r.mapr.tfdz.max_size = sizeof(map_buffer2);
	map_r.mapr.state = MAPR_STATE_BEGIN;
	mapp_init(&map_r, &map_mx_r, 5);

	pc_parse(&pc_r, pc_arr, sizeof(pc_arr));
	uint8_t arr3[40];
	quality_of_service_t qos = 0;
	int s = map_recieve(&map_r.mapr, arr3, sizeof(arr3), &qos);

	for (int i = 0; i < s; i++) {
		printf("0x%x ", arr3[i]);
	}

	printf("\n\nHELLO3\n");

/*
	uint8_t arr0[] = {0b11001010, 0b00110101};
	uint8_t arr1[4] = {0};

	uint8_t val = 0;
	for (int i = 3; i < 12; i++) {
		val = _stream_get_bit(arr0, 11, i, ENDIAN_LSBIT_MSBYTE);
		printf("%d", (int)val);
	}
	printf("\n");
	endian_stream_set(arr1, sizeof(arr1) * 8, 6, ENDIAN_LSBIT_LSBYTE,
					  arr0, 13, 1, ENDIAN_MSBIT_LSBYTE);
	print_bit_arr(arr1, sizeof(arr1));
*/
	return 0;
}



