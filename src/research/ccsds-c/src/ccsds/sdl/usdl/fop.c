#include <ccsds/sdl/usdl/fop.h>
#include <ccsds/sdl/usdl/usdl_types.h>
#include <string.h>
#include <assert.h>
#include <ccsds/sdl/usdl/usdl_debug.h>


int queue_push(queue_t *queue, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {

	assert(size <= FOP_DATA_SIZE);
	queue_element_t *t;
	if (!queue->empty) {
		return 0;
	}
	t = queue->empty;
	queue->empty = queue->empty->next;

	memcpy(t->value.data, data, size);
	t->value.map_params = *map_params;
	t->value.vc_params = *vc_params;
	t->value.size = size;
	queue->last->next = t;
	queue->last = t;

	if (!queue->first) {
		queue->first = queue->last;
	}
	return 1;
}

void queue_drop_first(queue_t *queue) {
	if (queue->first) {
		queue_element_t *v = queue->first;
		queue->first = queue->first->next;
		v->next = queue->empty;
		queue->empty = v;
		if (!queue->first) {
			queue->last = 0;
		}
	}
}
void queue_drop_all(queue_t *queue) {
	while (queue->first) {
		queue_element_t *v = queue->first;
		queue->first = queue->first->next;
		v->next = queue->empty;
		queue->empty = v;
	}
	queue->last = 0;

}

int queue_drop(queue_t *queue, vc_id_t vc_id) {
	queue_element_t **this = &queue->first;
	while (*this && (*this)->value.vc_params.vc_id != vc_id) {
		this = &(*this)->next;
	}
	if (*this) {
		*this = (*this)->next;
		return 1;
	} else {
		return 0;
	}

}
int queue_pop(queue_t *queue, uint8_t *data, size_t *size,
		map_params_t *map_params, vc_params_t *vc_params) {

	if (!queue->first) {
		return 0;
	}
	queue_value_t *t = &queue->first->value;

	if (queue->first->value.size > *size) {
		return 0;
	}

	memcpy(data, t->data, t->size);
	*size = t->size;
	*map_params = t->map_params;
	*vc_params = t->vc_params;

	queue_element_t *v = queue->first;
	queue->first = queue->first->next;
	v->next = queue->empty;
	queue->empty = v;
	return 1;
}
queue_element_t *queue_peek(queue_t *queue) {
	return queue->first;
}

int queue_is_full(queue_t *queue) {
	return queue->empty == 0;
}



int fop_add_packet(fop_t *fop, uint8_t *data, size_t size,
		map_params_t *map_params, vc_params_t *vc_params) {


	int ret = queue_push(&fop->queue, data, size, map_params, vc_params);
	if (ret) {
		queue_peek(&fop->queue)->value.vc_params.vcf_count = fop->frame_count++;
		return size;
	} else {
		return 0;
	}
}

queue_value_t *fop_peek(fop_t *fop) {
	queue_element_t *t = queue_peek(&fop->queue);
	if (t) {
		return &t->value;
	} else {
		return 0;
	}
}

void fop_drop(fop_t *fop) {
	queue_drop_first(&fop->queue);
}

int fop_control() {
	return 0;
}
