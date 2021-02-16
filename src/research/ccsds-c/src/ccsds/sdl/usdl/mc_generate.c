#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>

uint32_t _mc_generate_vcf_length(mc_t *mc);

int mc_init(mc_t *mc, pc_t *pc, mc_paramaters_t *params, mc_id_t mc_id) {
	usdl_print_debug();
	mc->pc = pc;
	mc->mc_id = mc_id;
	mc->pc->mc_arr[mc->mc_id] = mc;
	mc->mc_parameters = *params;
	mc->vcf_length = _mc_generate_vcf_length(mc);
	return 0;
}

int mc_generate(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	usdl_print_debug();

	mc_params_t p = {0};
	p.scid = mc->mc_parameters.scid;
	p.sod_flag = mc->mc_parameters.sod_flag;
	return pc_push(mc->pc, data, size, map_params, vc_params, &p);
}

int mc_push(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	vc_t **vc_arr = mc->vc_arr;
	if (mc->vc_mx->push(mc->vc_mx, vc_arr[vc_params->vc_id], (void **)vc_arr,
			sizeof(vc_arr) / sizeof(vc_arr[0]))) {
		return mc_generate(mc, data, size, map_params, vc_params);
	} else {
		return 0;
	}
}

int mc_request_from_down(mc_t *mc) {
	usdl_print_debug();
	return mc->vc_mx->pull(mc->vc_mx, (usdl_node_t **)mc->vc_arr, sizeof(mc->vc_arr) / sizeof(mc->vc_arr[0]));
}

uint32_t _mc_generate_vcf_length(mc_t *mc) {
	uint32_t size = mc->pc->mcf_length;
	if (mc->mc_parameters.ocf_flag == MC_OCF_PROHIBTED) {
		return size;
	} else {
		return size - 4;
	}

}

void mc_parse(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params, uint8_t *ocf) {
	usdl_print_debug();
	if (mc->vc_arr[vc_params->vc_id]) {
		vc_parse(mc->vc_arr[vc_params->vc_id], data, size, map_params, vc_params);
	}
}
