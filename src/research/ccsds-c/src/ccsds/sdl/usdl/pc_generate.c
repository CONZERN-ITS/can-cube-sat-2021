/*
 * p_generate.h
 *
 *  Created on: 16 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_PC_GENERATE_H_
#define SDL_USDL_PC_GENERATE_H_

#include <ccsds/sdl/usdl/pc_generate.h>
#include <ccsds/sdl/usdl/mc_multiplex.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/ccscds_endian.h>

int pc_generate(pc_t *pc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params) {

	if (pc->is_valid) {
		return 0;
	}
	// Заголовок
	ccsds_endian_insert(pc->data, pc->size * 8, 0, &pc->pc_parameters.tfvn, 4);
	ccsds_endian_insert(pc->data, pc->size * 8, 4, &mc_params->scid, 16);
	ccsds_endian_insert(pc->data, pc->size * 8, 20, &vc_params->sod_flag, 1);
	ccsds_endian_insert(pc->data, pc->size * 8, 21, &vc_params->vc_id, 6);
	ccsds_endian_insert(pc->data, pc->size * 8, 27, &map_params->map_id, 4);
	ccsds_endian_insert(pc->data, pc->size * 8, 31, &vc_params->eofph_flag, 1);

	int k = 32;
	if (!vc_params->eofph_flag) {
		ccsds_endian_insert(pc->data, pc->size * 8, k, &pc->pc_parameters.tf_length, 16);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 16, &vc_params->bsc_flag, 1);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 17, &vc_params->pcc_flag, 1);
		// Да, здесь должно быть 52
		ccsds_endian_insert(pc->data, pc->size * 8, k + 20, &mc_params->ocfp_flag, 1);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 21, &vc_params->vcf_count_length, 3);
		ccsds_endian_insert(pc->data, pc->size * 8, k + 24, &pc->pc_parameters.tf_length, 8 * (vc_params->vcf_count_length));
		k += 8 * (vc_params->vcf_count_length) + 24;
	}

	// Insert zone
	ccsds_endian_insert(pc->data, pc->size * 8, k, &pc->insert_data, pc->insert_size * 8);
	k += pc->insert_size * 8;

	//TFDF
	ccsds_endian_insert(pc->data, pc->size * 8, k, &map_params->rules, 3);
	ccsds_endian_insert(pc->data, pc->size * 8, k + 3, &map_params->upid, 5);
	k += 8;
	if (map_params->rules <= 2) {
		ccsds_endian_insert(pc->data, pc->size * 8, k , &map_params->fhd, 16);
		k += 16;
	}
	ccsds_endian_insert(pc->data, pc->size * 8, k, data, size * 8);
	k += size * 8;

	//OCF
	ccsds_endian_insert(pc->data, pc->size * 8, k, mc_params->ocf, 32);
	k += 32;

	//FEC
	if (pc->pc_parameters.is_fec_presented) {
		ccsds_endian_insert(pc->data, pc->size * 8, k, pc->fec_field, pc->pc_parameters.fec_length);
		k += pc->pc_parameters.fec_length * 8;
	}
	if (k != pc->pc_parameters.tf_length) {
		return 0;
	} else {
		pc->is_valid = 1;
		return pc->pc_parameters.tf_length;
	}
}

int pc_request_from_down(pc_t *pc) {
	return mc_mx_request_from_down(pc->mc_mx);
}
#endif /* SDL_USDL_PC_GENERATE_H_ */
