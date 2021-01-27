#include <ccsds/sdl/usdl/map_multiplexer.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <string.h>
#include <assert.h>
#include <ccsds/nl/epp/epp.h>
#include <ccsds/sdl/usdl/mapp_service.h>

static void mapp_parse(map_t *map, uint8_t *data, size_t size,
		map_params_t *map_params);

int mapp_request_from_down(map_t *map);

static uint32_t _mapp_generate_tfdz_length(map_t *map, quality_of_service_t qos);

int mapp_init(map_t *map, map_mx_t *mx, map_id_t map_id) {
	usdl_print_debug();
	map->map_parse = mapp_parse;
	map->map_request_from_down = mapp_request_from_down;

	map->map_mx = mx;
	map->map_id = map_id;
	map->map_mx->map_arr[map->map_id] = map;

	map->packet_qos = map->map_mx->vc->vc_parameters.cop_in_effect == COP_NONE ? QOS_EXPEDITED : QOS_SEQ_CONTROL;
	map->size_fixed = 1;
	map->split_allowed = 1;
	map->buf_ex.size = _mapp_generate_tfdz_length(map, QOS_EXPEDITED);
	map->buf_sc.size = _mapp_generate_tfdz_length(map, QOS_SEQ_CONTROL);
	return 0;
}



int mapp_send(map_t *map, uint8_t *data, size_t size, pvn_t pvn, quality_of_service_t qos) {
	usdl_print_debug();
#if MAP_BUFFER_SIZE == 0
	return 0;
#else
	assert(map->split_allowed);
	assert(map->size_fixed);
	map_buffer_t *buf = (qos == QOS_EXPEDITED ? &map->buf_ex : &map->buf_sc);

	map_params_t params = {0};
	params.qos = qos;
	params.map_id = map->map_id;
	params.rules = TFDZCR_SPAN;
	params.upid = UPID_SP_OR_EP;
	params.fhd = buf->fhd;

	if (pvn != PVN_ENCAPSULATION_PROTOCOL && pvn != PVN_SPACE_PACKET) {
		return 0;
	}

	if (buf->index == buf->size) {
		if (map_mx_multiplex(map->map_mx, &params, buf->data, buf->size)) {
			params.fhd = buf->fhd = 0;
			buf->index = 0;
		} else {
			return 0;
		}
	}

	if (buf->fhd == (uint16_t)-1) {
		params.fhd = buf->fhd = buf->index;
	}
	size_t data_index = 0;
	size_t count_to_send = 0;

	while (1) {
		count_to_send = buf->size - buf->index < size - data_index
				? buf->size - buf->index : size - data_index;
		if (count_to_send == 0) {
			break;
		}
		memcpy(&buf->data[buf->index], &data[data_index], count_to_send);
		buf->index += count_to_send;
		data_index += count_to_send;
		if (buf->index == buf->size &&
				map_mx_multiplex(map->map_mx, &params, buf->data, count_to_send)) {
			buf->index = 0;
			params.fhd = buf->fhd = (fhd_t)~0;
		} else {
			break;
		}
	}

	return data_index;
#endif
}

int mapp_request_from_down(map_t *map) {
	usdl_print_debug();
	map_buffer_t *buf = 0;

	map_params_t params = {0};

	if (map->buf_ex.index > 0) {
		params.qos = QOS_EXPEDITED;
		buf = &map->buf_ex;
	} else if (map->buf_sc.index > 0) {
		params.qos = QOS_SEQ_CONTROL;
		buf = &map->buf_sc;
	} else {
		return 0;
	}
	if (buf->index < buf->size) {
		epp_header_t head = {0};
		head.epp_id = EPP_ID_IDLE;
		epp_make_header_auto_length(&head, &buf->data[buf->index],
				buf->size - buf->index, buf->size - buf->index);
	}

	params.map_id = map->map_id;
	params.rules = TFDZCR_SPAN;
	params.upid = UPID_SP_OR_EP;
	params.fhd = buf->fhd;

	int ret = map_mx_multiplex(map->map_mx, &params, buf->data, buf->size);
	if (ret) {
		buf->fhd = 0;
		buf->index = 0;
	}
	return ret;
}

static pvn_t _map_get_pvn(uint8_t v) {
	return (v >> 5) & 7;
}

static int _map_try_calc_size(uint8_t *data, int size) {
	pvn_t pvn = _map_get_pvn(data[0]);
	if (pvn == PVN_ENCAPSULATION_PROTOCOL) {
		epp_header_t h = {0};
		if (epp_extract_header(&h, data, size)) {
			return h.packet_length;
		} else {
			return 0;
		}
	} else {
		return -1;
	}
}


static int _map_fhd_is_valid(mapr_t *map) {
	if (map->tfdz.fhd <= map->packet.index) {
		return 1;
	}
	if (map->tfdz.fhd == (fhd_t)~0) {
		return map->packet.size - map->packet.index >= map->tfdz.size - map->tfdz.index;
	}


	return map->packet.size - map->packet.index == map->tfdz.fhd - map->tfdz.index;
}

static int _map_push_from_down(mapr_t *map) {
	if (map->state == MAPR_STATE_FINISH) {
		return 0;
	}
	if (map->tfdz.index == map->tfdz.size) {
		return 0;
	}
	if (map->state == MAPR_STATE_BEGIN) {
		if (map->tfdz.fhd == (fhd_t)~0) {
			map->tfdz.index = map->tfdz.size;
			return 0;
		}

		map->tfdz.index = map->tfdz.fhd;
		map->state = MAPR_STATE_SIZE_UNKNOWN;
	}

	if (map->state == MAPR_STATE_SIZE_UNKNOWN) {
		while (map->tfdz.index < map->tfdz.size && map->packet.index < map->packet.max_size) {
			if (map->tfdz.index == map->tfdz.fhd) {
				map->packet.index = 0;
			}
			map->packet.data[map->packet.index++] = map->tfdz.data[map->tfdz.index++];
			int ret = _map_try_calc_size(map->packet.data, map->packet.index);

			if (ret > 0 && ret <= map->packet.max_size) {
				map->packet.size = ret;
				map->state = MAPR_STATE_SIZE_KNOWN;
				break;
			} else if (ret < 0 || ret > map->packet.max_size) {
				map->tfdz.index = map->tfdz.size;
				map->packet.index = 0;
				map->state = MAPR_STATE_BEGIN;
				break;
			}
		}
		if (map->packet.index == map->packet.max_size && map->state == MAPR_STATE_SIZE_UNKNOWN) {
			map->state = MAPR_STATE_SIZE_UNKNOWN;
			map->packet.index = 0;
		}
	}

	if (map->state == MAPR_STATE_SIZE_KNOWN) {
		if (!_map_fhd_is_valid(map)) {
			map->state = MAPR_STATE_BEGIN;
			map->packet.size = 0;
			map->packet.index = 0;
		}
		while (map->tfdz.index < map->tfdz.size && map->packet.index < map->packet.size) {
			map->packet.data[map->packet.index++] = map->tfdz.data[map->tfdz.index++];
		}
		if (map->packet.index == map->packet.size) {
			map->state = MAPR_STATE_FINISH;
			map->packet.index = 0;
			return 1;
		}
	}
	return 0;
}

static int _map_save_from_down(mapr_t *map,  const uint8_t *data, size_t size, const map_params_t *params) {

	if (map->tfdz.size != map->tfdz.index || map->tfdz.max_size < size) {
		return 0;
	} else {
		memcpy(map->tfdz.data, data, size);
		map->tfdz.size = size;
		map->tfdz.index = 0;
		map->tfdz.fhd = params->fhd;
		return size;
	}
}



static int map_recieve_from_down(mapr_t *map,  const uint8_t *data, size_t size, const map_params_t *params)  {
	_map_push_from_down(map);
	int ret = _map_save_from_down(map, data, size, params);
	_map_push_from_down(map);
	return ret;
}

int mapp_recieve(mapr_t *map, uint8_t *data, size_t size, quality_of_service_t *qos) {
	usdl_print_debug();
	if (map->state != MAPR_STATE_FINISH || size < map->packet.size) {
		return 0;
	}

	memcpy(data, map->packet.data, map->packet.size);
	map->state = MAPR_STATE_BEGIN;
	map->packet.index = 0;
	return map->packet.size;
}


static uint32_t _mapp_generate_tfdz_length(map_t *map, quality_of_service_t qos) {
	uint32_t size = 0;
	if (qos == QOS_EXPEDITED) {
		size = map->map_mx->vc->mapcf_length_ex;
	} else {
		size = map->map_mx->vc->mapcf_length_sc;
	}
	return size - 3;
}

static void mapp_parse(map_t *map, uint8_t *data, size_t size,
		map_params_t *map_params) {
	usdl_print_debug();
	map_recieve_from_down(&map->mapr, data, size, map_params);
}


