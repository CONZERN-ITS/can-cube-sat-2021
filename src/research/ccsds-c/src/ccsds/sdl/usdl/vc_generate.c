#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/map_service_abstract.h>
#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <string.h>
#include <assert.h>

uint32_t _vc_generate_mapcf_length(vc_t *vc, quality_of_service_t qos);

int vc_init(vc_t *vc, mc_t *mc, const vc_parameters_t *params, vc_id_t vc_id) {
	usdl_print_debug();
	vc->mc = mc;
	vc->vc_id = vc_id;
	vc->mc->vc_arr[vc->vc_id] = vc;
	vc->vc_parameters = *params;
	vc->mapcf_length_ex = _vc_generate_mapcf_length(vc, QOS_EXPEDITED);
	vc->mapcf_length_sc = _vc_generate_mapcf_length(vc, QOS_SEQ_CONTROL);
	return 0;
}
int _vc_pop_fop(vc_t *vc) {
	queue_value_t *v = fop_peek(&vc->fop);
	int ret = mc_push(vc->mc, v->data, v->size, &v->map_params, &v->vc_params);
	if (ret) {
		fop_drop(&vc->fop);
	}

	return ret;
}


int vc_generate(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size) {
	usdl_print_debug();
	vc_params_t p = {};
	if (map_params->qos == QOS_SEQ_CONTROL) {
		p.bsc_flag = BSC_SEQ_CONTROL;
		p.vcf_count_length = vc->vc_parameters.vcf_count_lenght_seq_control;
		p.vcf_count = vc->frame_count_seq_control;
		p.pcc_flag = PCC_USER_DATA; //TODO Надо понять, как тут должно быть@
		p.eofph_flag = EOFPH_FULL;
		p.vc_id = vc->vc_id;
	} else {
		p.bsc_flag = BSC_EXPEDITED;
		p.vcf_count_length = vc->vc_parameters.vcf_count_lenght_expedited;
		p.vcf_count = vc->frame_count_expedited;
		p.pcc_flag = PCC_USER_DATA; //TODO Надо понять, как тут должно быть
		p.eofph_flag = EOFPH_FULL;
		p.vc_id = vc->vc_id;
	}

	if (vc->vc_parameters.cop_in_effect == COP_1) {
		_vc_pop_fop(vc);
		int ret = fop_add_packet(&vc->fop, data, size, map_params, &p);
		while (_vc_pop_fop(vc));
		return ret;
	} else if (vc->vc_parameters.cop_in_effect == COP_NONE) {
		return mc_push(vc->mc, data, size, map_params, &p);
	} else {
		return 0;
	}
}

int vc_push(vc_t *vc, map_params_t *map_params, uint8_t *data, size_t size) {
	map_t **maps = vc->map_arr;
	if (vc->map_mx->push(vc->map_mx, (usdl_node_t *)maps[map_params->map_id], (usdl_node_t **)maps,
			sizeof(maps) / sizeof(maps[0]))) {
		return vc_generate(vc, map_params, data, size);
	} else {
		return 0;
	}
}

int vc_request_from_down(vc_t *vc) {
	usdl_print_debug();
	if (vc->vc_parameters.cop_in_effect == COP_1) {
		int ret = _vc_pop_fop(vc);
		if (ret) {
			return ret;
		} else {
			int ret = vc->map_mx->pull(vc->map_mx, (usdl_node_t **)vc->map_arr, sizeof(vc->map_arr) / sizeof(vc->map_arr[0]));
			if (ret) {
				return _vc_pop_fop(vc);
			} else {
				return 0;
			}
		}
	} else if (vc->vc_parameters.cop_in_effect == COP_NONE) {
		return vc->map_mx->pull(vc->map_mx, (usdl_node_t **)vc->map_arr, sizeof(vc->map_arr) / sizeof(vc->map_arr[0]));
	} else {
		return 0;
	}
}


uint32_t _vc_generate_mapcf_length(vc_t *vc, quality_of_service_t qos) {
	uint32_t size = vc->mc->vcf_length;
	size -= 7;
	if (qos == QOS_EXPEDITED) {
		size -= vc->vc_parameters.vcf_count_lenght_expedited;
	} else {
		size -= vc->vc_parameters.vcf_count_lenght_seq_control;
	}
	return size;
}


void vc_parse(vc_t *vc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	usdl_print_debug();
	if (vc->map_arr[map_params->map_id]) {
		map_parse(vc->map_arr[map_params->map_id], data, size, map_params);
	}
}
