#include <ccsds/sdl/usdl/vc_multiplex.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>

int vc_mx_init(vc_mx_t *vc_mx, mc_t *mc) {
	vc_mx->mc = mc;
	vc_mx->mc->vc_mx = vc_mx;
	return 0;
}

int vc_multiplex(vc_mx_t *vc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	if (vc_params->vc_id != vc_mx->vc_active) {
		return 0;
	} else {
		int ret = mc_generate(vc_mx->mc, data, size, map_params, vc_params);
		if (ret) {
			mc_id_t old = vc_mx->vc_active;
			do {
				vc_mx->vc_active++;
			} while (vc_mx->vc_active != old && vc_mx->vc_arr[vc_mx->vc_active] == 0);
		}
		return ret;
	}
}

int vc_mx_request_from_down(vc_mx_t *vc_mx) {

	vc_id_t old = vc_mx->vc_active;
	do {
		if (vc_mx->vc_arr[vc_mx->vc_active]) {
			int ret = vc_request_from_down(vc_mx->vc_arr[vc_mx->vc_active]);
			if (ret) {
				return ret;
			}
		}

		vc_mx->vc_active = (vc_mx->vc_active + 1) %
				(sizeof(vc_mx->vc_arr) / sizeof(vc_mx->vc_arr[0]));
	} while (vc_mx->vc_active != old);

	return 0;
}

void vc_demultiplex(vc_mx_t *vc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {
	usdl_print_debug();
	if (vc_mx->vc_arr[map_params->map_id]) {
		vc_parse(vc_mx->vc_arr[map_params->map_id], data, size, map_params, vc_params);
	}
}
