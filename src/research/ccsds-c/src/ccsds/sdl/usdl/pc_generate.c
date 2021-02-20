#include <ccsds/sdl/usdl/pc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/ccscds_endian.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <string.h>

uint32_t _pc_generate_mcf_length(pc_t *pc);

int pc_init(pc_t *pc, const pc_paramaters_t *params,  uint8_t *data, size_t size) {
	usdl_print_debug();
	pc->pc_parameters = *params;
	pc->data = data;
	pc->size = size;
	pc->is_valid = 0;
	pc->insert_size = 0;
	pc->insert_data = 0;
	pc->mcf_length = _pc_generate_mcf_length(pc);
	return 0;
}

int pc_generate(pc_t *pc, const uint8_t *data, size_t size,
		const map_params_t *map_params, const vc_params_t *vc_params, const mc_params_t *mc_params) {
	usdl_print_debug();

	if (pc->is_valid) {
		return 0;
	}
	pc->size = pc->pc_parameters.tf_length;
	// Заголовок
	ccsds_endian_insert(pc->data, pc->size * 8, 0, &pc->pc_parameters.tfvn, 4);
	ccsds_endian_insert(pc->data, pc->size * 8, 4, &mc_params->scid, 16);
	ccsds_endian_insert(pc->data, pc->size * 8, 20, &mc_params->sod_flag, 1);
	ccsds_endian_insert(pc->data, pc->size * 8, 21, &vc_params->vc_id, 6);
	ccsds_endian_insert(pc->data, pc->size * 8, 27, &map_params->map_id, 4);
	ccsds_endian_insert(pc->data, pc->size * 8, 31, &vc_params->eofph_flag, 1);

	int k = 32;
	if (!vc_params->eofph_flag) {
		uint16_t s = pc->pc_parameters.tf_length - 1;
		ccsds_endian_insert(pc->data, pc->size * 8, k, &s, 16);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 16, &vc_params->bsc_flag, 1);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 17, &vc_params->pcc_flag, 1);
		// Да, здесь должно быть 20
		ccsds_endian_insert(pc->data, pc->size * 8, k + 20, &mc_params->ocfp_flag, 1);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 21, &vc_params->vcf_count_length, 3);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 24, &vc_params->vcf_count, 8 * (vc_params->vcf_count_length));
		k += 8 * (vc_params->vcf_count_length) + 24;
	}

	// Insert zone
	if (pc->insert_size) {
		memcpy(&pc->data[k / 8], pc->insert_data, pc->insert_size);
		k += pc->insert_size * 8;
	}

	//TFDF
	ccsds_endian_insert(pc->data, pc->size * 8, k, &map_params->rules, 3);
	ccsds_endian_insert(pc->data, pc->size * 8, k + 3, &map_params->upid, 5);
	k += 8;
	if (map_params->rules <= 2) {
		ccsds_endian_insert(pc->data, pc->size * 8, k , &map_params->fhd, 16);
		k += 16;
	}
	if (size) {
		memcpy(&pc->data[k / 8], data, size);
		k += size * 8;
	}

	//OCF
	if (mc_params->ocfp_flag) {
		memcpy(&pc->data[k / 8], mc_params->ocf, 4);
		k += 32;
	}

	//FEC
	if (pc->pc_parameters.is_fec_presented) {
		memcpy(&pc->data[k / 8], pc->fec_field, pc->pc_parameters.fec_length);
		k += pc->pc_parameters.fec_length * 8;
	}
	if (k / 8 != pc->pc_parameters.tf_length && pc->pc_parameters.tft == TF_FIXED) {
		return 0;
	} else if (k > pc->pc_parameters.tf_length && pc->pc_parameters.tft == TF_VARIABLE) {
		return 0;
	} else {
		pc->is_valid = 1;
		return pc->pc_parameters.tf_length;
	}
}

int pc_parse(pc_t *pc, uint8_t *data, size_t size) {
	usdl_print_debug();
	uint8_t *payload = 0;
	uint8_t *insert_zone;
	size_t pl_size = 0;
	map_params_t map_params = {0};
	vc_params_t vc_params = {0};
	mc_params_t mc_params = {0};
	uint64_t vcf_count = 0;
	uint16_t tf_length = 0;
	uint8_t *ocf_pointer = 0;
	// Заголовок
	uint8_t tfvn = 0;
	ccsds_endian_extract(data, 0, &tfvn, 4);
	if (pc->pc_parameters.tfvn != tfvn) {
		return -1;
	}
	ccsds_endian_extract(data, 4, &mc_params.scid, 16);
	ccsds_endian_extract(data, 20, &mc_params.sod_flag, 1);
	ccsds_endian_extract(data, 21, &vc_params.vc_id, 6);
	ccsds_endian_extract(data, 27, &map_params.map_id, 4);
	ccsds_endian_extract(data, 31, &vc_params.eofph_flag, 1);

	int k = 32;
	if (!vc_params.eofph_flag) {
		ccsds_endian_extract(data, k, &tf_length, 16);
		if ((pc->pc_parameters.tft == TF_FIXED && tf_length != pc->pc_parameters.tf_length - 1) ||
				(pc->pc_parameters.tft == TF_VARIABLE && tf_length > pc->pc_parameters.tf_length - 1)) {
			return -1;
		}
		ccsds_endian_extract(data, k + 16, &vc_params.bsc_flag, 1);
		ccsds_endian_extract(data, k + 17, &vc_params.pcc_flag, 1);
		// Да, здесь должно быть 20
		ccsds_endian_extract(data, k + 20, &mc_params.ocfp_flag, 1);
		ccsds_endian_extract(data, k + 21, &vc_params.vcf_count_length, 3);
		ccsds_endian_extract(data, k + 24, &vcf_count, 8 * (vc_params.vcf_count_length));
		k += 8 * (vc_params.vcf_count_length) + 24;
	}

	//insert zone
	insert_zone = &data[k / 8];
	k += pc->insert_size * 8;

	//TFDF
	ccsds_endian_extract(data, k, &map_params.rules, 3);
	ccsds_endian_extract(data, k + 3, &map_params.upid, 5);
	k += 8;
	if (map_params.rules <= 2) {
		ccsds_endian_extract(data, k , &map_params.fhd, 16);
		k += 16;
	}
	payload = &data[k / 8];


	int m = (tf_length + 1) * 8;
	//FEC
	if (pc->pc_parameters.is_fec_presented) {
		m -= pc->pc_parameters.fec_length * 8;
		ccsds_endian_extract(data, m,
				pc->fec_field, pc->pc_parameters.fec_length * 8);
	}

	//OCF
	if (mc_params.ocfp_flag == OCFP_PRESENT) {
		m -= 32;
		ccsds_endian_extract(data, m, mc_params.ocf, 32);
	}

	pl_size = (m - k) / 8;

	if (pc->mc_arr[mc_params.mc_id]) {
		mc_parse(pc->mc_arr[mc_params.mc_id], payload, pl_size, &map_params, &vc_params, &mc_params, ocf_pointer);
	}
	return 1;
}

int pc_push(pc_t *pc, const uint8_t *data, size_t size,
		const map_params_t *map_params, const vc_params_t *vc_params, const mc_params_t *mc_params) {
	mc_t **mc_arr = pc->mc_arr;
	if (pc->mc_mx->push(pc->mc_mx, (usdl_node_t *)mc_arr[mc_params->mc_id], (usdl_node_t **)mc_arr,
			sizeof(pc->mc_arr) / sizeof(pc->mc_arr[0]))) {
		return pc_generate(pc, data, size, map_params, vc_params, mc_params);
	} else {
		return 0;
	}
}

int pc_request_from_down(pc_t *pc) {
	usdl_print_debug();
	return pc->mc_mx->pull(pc->mc_mx, (usdl_node_t **)pc->mc_arr, sizeof(pc->mc_arr) / sizeof(pc->mc_arr[0]));
}

uint32_t _pc_generate_mcf_length(pc_t *pc) {
	uint32_t size = pc->pc_parameters.tf_length;
	if (pc->pc_parameters.is_fec_presented) {
		size -= pc->pc_parameters.fec_length;
	}
	size -= pc->insert_size;
	return size;
}
