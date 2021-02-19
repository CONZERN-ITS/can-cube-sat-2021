#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <ccsds/sdl/usdl/vc_generate.h>
#include <string.h>
#include <assert.h>
#include <ccsds/nl/epp/epp.h>



static void mapa_parse(map_t *map, uint8_t *data, size_t size,
		map_params_t *map_params);

static int mapa_request_from_down(usdl_node_t *node);

static uint32_t _mapa_generate_tfdz_length(map_t *map, quality_of_service_t qos);

int mapa_init(map_t *map, vc_t *vc, map_id_t map_id) {
	usdl_print_debug();
	map->map_parse = mapa_parse;
	map->map_request_from_down = mapa_request_from_down;
	map->base.request_from_down = mapa_request_from_down;

	map->vc = vc;
	map->map_id = map_id;
	map->vc->map_arr[map->map_id] = map;
	map->mapr.state = MAPR_STATE_BEGIN;

	map->packet_qos = map->vc->vc_parameters.cop_in_effect == COP_NONE ? QOS_EXPEDITED : QOS_SEQ_CONTROL;
	map->size_fixed = 1;
	map->split_allowed = 1;
	map->buf_ex.size = _mapa_generate_tfdz_length(map, QOS_EXPEDITED);
	map->buf_sc.size = _mapa_generate_tfdz_length(map, QOS_SEQ_CONTROL);
	map->buf_ex.rules = TFDZCR_SDU_START;
	map->buf_sc.rules = TFDZCR_SDU_START;
	return 0;
}



int mapa_send(map_t *map, const uint8_t *data, size_t size, quality_of_service_t qos) {
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
	params.rules = buf->rules;
	params.upid = UPID_MAPA_SDU;
	params.lvo = buf->lvo;


	size_t data_index = 0;
	size_t count_to_send = 0;

	while (1) {
		if (buf->index != 0) {
			if (vc_push(map->vc, &params, buf->data, buf->size)) {

				if (buf->lvo == (lvo_t)~0) {
					params.rules = buf->rules = TFDZCR_SDU_CONT;
				} else {
					params.rules = buf->rules = TFDZCR_SDU_START;
				}

				buf->index = 0;
				params.lvo = buf->lvo = (fhd_t)~0;
			} else {
				break;
			}
		}

		count_to_send = buf->size < size - data_index
				? buf->size : size - data_index;

		if (count_to_send == 0) {
			break;
		}
		memcpy(buf->data, &data[data_index], count_to_send);
		buf->index += count_to_send;
		data_index += count_to_send;
		if (data_index == size) {
			params.lvo = buf->lvo = buf->index - 1;
			if (buf->size - buf->index > 0) {
				memset(&buf->data[buf->index], 0, buf->size - buf->index);
				buf->index = buf->size;
			}
		} else {
			params.lvo = buf->lvo = (lvo_t)~0;

		}
	}
	return data_index;
#endif
}


static int mapa_request_from_down(usdl_node_t *node) {
	usdl_print_debug();
	map_t *map = node;
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

	params.map_id = map->map_id;
	params.rules = buf->rules;
	params.upid = UPID_MAPA_SDU;
	params.lvo = buf->lvo;

	int ret = vc_push(map->vc, &params, buf->data, buf->size);
	if (ret) {
		buf->lvo = (lvo_t)~0;
		buf->index = 0;
	}
	return ret;
}


static uint32_t _mapa_generate_tfdz_length(map_t *map, quality_of_service_t qos) {
	uint32_t size = 0;
	if (qos == QOS_EXPEDITED) {
		size = map->vc->mapcf_length_ex;
	} else {
		size = map->vc->mapcf_length_sc;
	}
	return size - 3;
}


static int _map_push_from_down(mapr_t *map) {
	if (map->state == MAPR_STATE_FINISH) {
		return 0;
	}
	if (map->tfdz.index == map->tfdz.size) {
		return 0;
	}
	if (map->rules == TFDZCR_SDU_START) {
		map->state = MAPR_STATE_BEGIN;
	}
	if (map->state == MAPR_STATE_BEGIN) {
		if (map->rules != TFDZCR_SDU_START) {
			map->tfdz.index = map->tfdz.size;
			return 0;
		}
		map->state = MAPR_STATE_SIZE_UNKNOWN;
	}

	if (map->state == MAPR_STATE_SIZE_UNKNOWN) {
		size_t count_to_send = map->tfdz.size;
		if (map->tfdz.lvo != (lvo_t)~0) {
			count_to_send = map->tfdz.lvo + 1;
		}
		if (map->packet.max_size - map->packet.index < count_to_send) {
			map->state = MAPR_STATE_BEGIN;
			map->packet.index = 0;
		}

		memcpy(&map->packet.data[map->packet.index], map->tfdz.data, count_to_send);
		map->packet.index += count_to_send;
		map->tfdz.index = map->tfdz.size;
		if (map->tfdz.lvo != (lvo_t)~0) {
			map->state = MAPR_STATE_FINISH;
			map->packet.size = map->packet.index;
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
		map->tfdz.lvo = params->lvo;
		map->rules = params->rules;
		return size;
	}
}



static int map_recieve_from_down(mapr_t *map,  const uint8_t *data, size_t size, const map_params_t *params)  {
	_map_push_from_down(map);
	int ret = _map_save_from_down(map, data, size, params);
	_map_push_from_down(map);
	return ret;
}

int mapa_recieve(mapr_t *map, uint8_t *data, size_t size, quality_of_service_t *qos) {
	usdl_print_debug();
	if (map->state != MAPR_STATE_FINISH || size < map->packet.size) {
		return 0;
	}

	memcpy(data, map->packet.data, map->packet.size);
	map->state = MAPR_STATE_BEGIN;
	map->packet.index = 0;
	return map->packet.size;
}


static void mapa_parse(map_t *map, uint8_t *data, size_t size,
		map_params_t *map_params) {
	usdl_print_debug();
	map_recieve_from_down(&map->mapr, data, size, map_params);
}
