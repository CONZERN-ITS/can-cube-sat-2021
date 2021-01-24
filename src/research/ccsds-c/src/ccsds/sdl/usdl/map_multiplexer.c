#include <ccsds/sdl/usdl/map_multiplexer.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <string.h>
#include <assert.h>
#include <ccsds/sdl/usdl/mapp_service.h>


int map_mx_init(map_mx_t *map_mx, vc_t *vc) {
	usdl_print_debug();
	map_mx->vc = vc;
	map_mx->vc->map_mx = map_mx;
	return 0;
}


int map_mx_multiplex(map_mx_t *map_mx, map_params_t *map_params, uint8_t *data, size_t size) {
	usdl_print_debug();
	if (map_params->map_id != map_mx->map_active) {
		return 0;
	} else {
		int ret = vc_generate(map_mx->vc, map_params, data, size);
		if (ret) {
			map_id_t old = map_mx->map_active;
			do {
				map_mx->map_active = (map_mx->map_active + 1) %
						(sizeof(map_mx->map_arr) / sizeof(map_mx->map_arr[0]));
			} while (map_mx->map_active != old && map_mx->map_arr[map_mx->map_active] == 0);
		}
		return ret;
	}
}

int map_mx_request_from_down(map_mx_t *map_mx) {
	usdl_print_debug();

	map_id_t old = map_mx->map_active;
	do {
		if (map_mx->map_arr[map_mx->map_active]) {
			int ret = map_request_from_down(map_mx->map_arr[map_mx->map_active]);
			if (ret) {
				return ret;
			}
		}

		map_mx->map_active = (map_mx->map_active + 1) %
				(sizeof(map_mx->map_arr) / sizeof(map_mx->map_arr[0]));
	} while (map_mx->map_active != old);

	return 0;
}

void map_demultiplex(map_mx_t *map_mx, uint8_t *data, size_t size,
		map_params_t *map_params) {
	usdl_print_debug();
	if (map_mx->map_arr[map_params->map_id]) {
		map_parse(map_mx->map_arr[map_params->map_id], data, size, map_params);
	}
}

