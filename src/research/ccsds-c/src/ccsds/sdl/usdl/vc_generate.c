/*
 * vc_generation.h
 *
 *  Created on: 9 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_VC_GENERATE_H_
#define SDL_USDL_VC_GENERATE_H_

#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/map_multiplexer.h>
#include <ccsds/sdl/usdl/vc_multiplex.h>
#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <string.h>
#include <assert.h>

int _vc_fop() {
	return 0;
}

int _vc_pop_fop(vc_t *vc) {
	queue_value_t *v = fop_peek(&vc->fop);
	int ret = vc_multiplex(vc->vc_mx, v->data, v->size, &v->map_params, &v->vc_params);
	if (ret) {
		fop_drop(&vc->fop);
	}

	return ret;
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
		_vc_pop_fop(vc);
		int ret = fop_add_packet(&vc->fop, data, size, map_params, &p);
		while (_vc_pop_fop(vc));
		return ret;
	} else if (vc->parameters.cop_in_effect == COP_NONE) {
		return vc_multiplex(vc->vc_mx, data, size, map_params, &p);
	} else {
		return 0;
	}
}





#endif /* SDL_USDL_VC_GENERATE_H_ */
