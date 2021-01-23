#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/vc_multiplex.h>
#include <ccsds/sdl/usdl/mc_multiplex.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>

uint32_t _mc_generate_vcf_length(mc_t *mc);

int mc_init(mc_t *mc, mc_mx_t *mx, mc_paramaters_t *params, mc_id_t mc_id) {
	usdl_print_debug();
	mc->mc_mx = mx;
	mc->mc_id = mc_id;
	mc->mc_mx->mc_arr[mc->mc_id] = mc;
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
	return mc_multiplex(mc->mc_mx, data, size, map_params, vc_params, &p);
}

int mc_request_from_down(mc_t *mc) {
	usdl_print_debug();
	return vc_mx_request_from_down(mc->vc_mx);
}

uint32_t _mc_generate_vcf_length(mc_t *mc) {
	uint32_t size = mc->mc_mx->pc->mcf_length;
	if (mc->mc_parameters.ocf_flag == MC_OCF_PROHIBTED) {
		return size;
	} else {
		return size - 4;
	}

}

void mc_parse(mc_t *mc, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params, uint8_t *ocf) {
	usdl_print_debug();
	vc_demultiplex(mc->vc_mx, data, size, map_params, vc_params);
}
