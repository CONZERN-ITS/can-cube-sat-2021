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
#include <stdlib.h>

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

//#define MAIN_MAPP

typedef enum {
	USLPS_MAPP,
	USLPS_MAPA,
	USLPS_VCF,
} uslp_service_t;

typedef struct uslp_t {
	pc_t pc;
	mc_mx_t mc_mx;
	mc_t mc;
	vc_mx_t vc_mx;
	vc_t vc;
	map_mx_t map_mx;
	map_t map;
} uslp_t;

void main_uslp_init(uslp_t *uslp, size_t tf_length, uslp_service_t service, size_t max_packet_size) {
	pc_paramaters_t pc_p = {0};
	pc_p.can_generate_oid = 0;
	pc_p.is_fec_presented = 0;
	pc_p.tf_length = tf_length;
	pc_p.tft = TF_FIXED;
	pc_p.tfvn = 0x0C;
	uint8_t *pc_arr = malloc(tf_length);
	pc_init(&uslp->pc, &pc_p, pc_arr, tf_length);

	mc_mx_init(&uslp->mc_mx, &uslp->pc);

	mc_paramaters_t mc_p = {0};
	mc_p.ocf_flag = 0;
	mc_p.scid = 0x1234;
	mc_p.tft = TF_FIXED;
	mc_init(&uslp->mc, &uslp->mc_mx, &mc_p, 0);

	vc_mx_init(&uslp->vc_mx, &uslp->mc);

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
	vc_init(&uslp->vc, &uslp->vc_mx, &vc_p, 0);

	map_mx_init(&uslp->map_mx, &uslp->vc);

	uint8_t *map_buffer = malloc(tf_length);
	uint8_t *map_buffer_p = malloc(max_packet_size);
	uslp->map.buf_ex.data = map_buffer;
	uslp->map.buf_ex.max_size = tf_length;
	uslp->map.mapr.tfdz.data = map_buffer;
	uslp->map.mapr.tfdz.max_size = tf_length;
	uslp->map.mapr.packet.data = map_buffer_p;
	uslp->map.mapr.packet.max_size = max_packet_size;
	if (service == USLPS_MAPA) {
		mapa_init(&uslp->map, &uslp->map_mx, 5);
	} else {
		mapp_init(&uslp->map, &uslp->map_mx, 5);
	}
}

void test_send_recieve(uslp_t *us, uslp_t *ur, uint8_t *data, size_t size, uslp_service_t service) {
	int s = 0;
	int sent = 0;
	int recieved = 0;
	while (sent < size || recieved < size) {
		if (service == USLPS_MAPP) {
			s = mapp_send(&us->map, &data[sent], size - sent, PVN_ENCAPSULATION_PROTOCOL, QOS_EXPEDITED);
		} else {
			s =	mapa_send(&us->map, &data[sent], size - sent, QOS_EXPEDITED);
		}


		sent += s;
		pc_request_from_down(&us->pc);
		if (us->pc.is_valid) {
			for (int i = 0; i < us->pc.pc_parameters.tf_length && s; i++) {
				printf("0x%x ", us->pc.data[i]);
			}
			printf("\n");
			if (pc_parse(&ur->pc, us->pc.data, us->pc.pc_parameters.tf_length)) {
				us->pc.is_valid = 0;
			}
		}

		quality_of_service_t qos = 0;
		if (service == USLPS_MAPP) {
			s = mapp_recieve(&ur->map.mapr, data, size, &qos);
		} else {
			s = mapa_recieve(&ur->map.mapr, data, size, &qos);
		}
		recieved += s;
		if (s) {
			break;
		}
	}
	if (s) {
		for (int i = 0; i < s; i++) {
			printf("0x%x ", data[i]);
		}
		printf("\n");
	}

}

int main() {
	const int size = 0x80;
	uint8_t pl_arr[size];
	int pl_size = size - 4;
	const uslp_service_t test_service = USLPS_MAPA;
	printf("HELLO\n");
	uslp_t us = {0};
	uslp_t ur = {0};
	main_uslp_init(&us, 30, test_service, size);
	main_uslp_init(&ur, 30, test_service, size);


	epp_header_t epp_header = {0};
	epp_header.epp_id = 1;


	if (test_service == USLPS_MAPP) {
		int epp_size = epp_make_header_auto_length2(&epp_header, pl_arr, sizeof(pl_arr), pl_size);
		for (int i = 0; i < pl_size; i++) {
			pl_arr[i + epp_size] = i + 1;
		}
		pl_size += epp_size;
	} else {
		for (int i = 0; i < pl_size; i++) {
			pl_arr[i] = i + 1;
		}
	}

	test_send_recieve(&us, &ur, pl_arr, pl_size, test_service);
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



