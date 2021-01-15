/*
 * vc_generation.h
 *
 *  Created on: 9 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_GENERATION_H_
#define SDL_USDL_VC_GENERATION_H_

#include "usdl_types.h"
#include "fop.h"

int _vc_fop() {

}

int vc_generate(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size) {
	vc_params_t p = {};
	if (map_params->qos == QOS_SEQ_CONTROL) {
		p.bsc_flag = BSC_SEQ_CONTROL;
		p.vcf_count_length = vc->parameters.vcf_count_lenght_seq_control;
		p.vcf_count = vc->frame_count_seq_control;
		p.pcc_flag = PCC_USER_DATA; //TODO Надо понять, как тут должно быть
		p.sod_flag = vc->src_or_dest_id;
		p.vc_id = vc->vc_id;
	} else {
		p.bsc_flag = BSC_EXPEDITED;
		p.vcf_count_length = vc->parameters.vcf_count_lenght_expedited;
		p.vcf_count = vc->frame_count_expedited;
		p.pcc_flag = PCC_USER_DATA; //TODO Надо понять, как тут должно быть
		p.sod_flag = vc->src_or_dest_id;
		p.vc_id = vc->vc_id;
	}

	if (vc->parameters.cop_in_effect == COP_1) {

		return fop_add_packet(vc->fop, data, size, map_params, &p);
	} else if (vc->parameters.cop_in_effect == COP_NONE) {

	}
}





#endif /* SDL_USDL_VC_GENERATION_H_ */
