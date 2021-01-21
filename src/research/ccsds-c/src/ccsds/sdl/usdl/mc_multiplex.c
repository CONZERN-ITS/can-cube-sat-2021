#include <ccsds/sdl/usdl/mc_multiplex.h>
#include <ccsds/sdl/usdl/mc_generate.h>
#include <ccsds/sdl/usdl/pc_generate.h>
#include <ccsds/sdl/usdl/usdl_types.h>

int mc_mx_init(mc_mx_t *mc_mx, pc_t *pc) {
	mc_mx->pc = pc;
	mc_mx->pc->mc_mx = mc_mx;
	return 0;
}

int mc_multiplex(mc_mx_t *mc_mx, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params, mc_params_t *mc_params) {
	if (mc_params->mc_id != mc_mx->mc_active) {
		return 0;
	} else {
		int ret = pc_generate(mc_mx->pc, data, size, map_params, vc_params, mc_params);
		if (ret) {
			mc_id_t old = mc_mx->mc_active;
			do {
				mc_mx->mc_active++;
			} while (mc_mx->mc_active != old && mc_mx->mc_arr[mc_mx->mc_active] == 0);
		}
		return ret;
	}
}

int mc_mx_request_from_down(mc_mx_t *mc_mx) {

	mc_id_t old = mc_mx->mc_active;
	do {
		if (mc_mx->mc_arr[mc_mx->mc_active]) {
			int ret = mc_request_from_down(mc_mx->mc_arr[mc_mx->mc_active]);
			if (ret) {
				return ret;
			}
		}

		mc_mx->mc_active = (mc_mx->mc_active + 1) %
				(sizeof(mc_mx->mc_arr) / sizeof(mc_mx->mc_arr[0]));
	} while (mc_mx->mc_active != old);

	return 0;
}

