#include <ccsds/sdl/usdl/map_multiplexer.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <ccsds/sdl/usdl/usdl_debug.h>
#include <string.h>
#include <assert.h>
#include <ccsds/nl/epp/epp.h>


static uint32_t _mapa_generate_tfdz_length(map_t *map, quality_of_service_t qos);

int mapa_init(map_t *map, map_mx_t *mx, map_id_t map_id) {
	usdl_print_debug();
	map->map_mx = mx;
	map->map_id = map_id;
	map->map_mx->map_arr[map->map_id] = map;

	map->packet_qos = map->map_mx->vc->vc_parameters.cop_in_effect == COP_NONE ? QOS_EXPEDITED : QOS_SEQ_CONTROL;
	map->size_fixed = 1;
	map->split_allowed = 1;
	map->buf_ex.size = _mapa_generate_tfdz_length(map, QOS_EXPEDITED);
	map->buf_sc.size = _mapa_generate_tfdz_length(map, QOS_SEQ_CONTROL);
	return 0;
}



int mapa_send(map_t *map, uint8_t *data, size_t size, quality_of_service_t qos) {
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
	params.rules = map->flags;
	params.upid = UPID_SP_OR_EP;
	params.lvo = buf->lvo;


	if (buf->index == buf->size || buf->lvo != (lvo_t)~0) {
		if (map_mx_multiplex(map->map_mx, &params, buf->data, buf->size)) {
			if (buf->lvo == (lvo_t)~0) {
				params.rules = map->flags = TFDZCR_SDU_CONT;
			} else {
				params.rules = map->flags = TFDZCR_SDU_START;
			}
			buf->index = 0;
		} else {
			return 0;
		}
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
		if (data_index == size) {
			params.lvo = buf->lvo = buf->index - 1;
			if (buf->size - buf->index) {
				memset(&buf->data[buf->index], 0, buf->size - buf->index);
				buf->index = buf->size;
			}
		}
		if (buf->index == buf->size &&
				map_mx_multiplex(map->map_mx, &params, buf->data, count_to_send)) {
			buf->index = 0;
			params.lvo = buf->lvo = (fhd_t)~0;
		} else {
			break;
		}
	}

	return data_index;
#endif
}

static uint32_t _mapa_generate_tfdz_length(map_t *map, quality_of_service_t qos) {
	uint32_t size = 0;
	if (qos == QOS_EXPEDITED) {
		size = map->map_mx->vc->mapcf_length_ex;
	} else {
		size = map->map_mx->vc->mapcf_length_sc;
	}
	return size - 3;
}
