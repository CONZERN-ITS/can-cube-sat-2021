/*
 * multiplexer.c
 *
 *  Created on: 16 февр. 2021 г.
 *      Author: HP
 */

#include <ccsds/sdl/usdl/multiplexer.h>

#define NEXT(id, size) (((id) + 1) % (size))
#define PREV(id, size) (((id) + (size) - 1) % (size))

int mx_push(struct multiplexer_abstract *mx_a, usdl_node_t *p, usdl_node_t **arr, int size) {
	assert(p != 0);
	struct multiplexer_rr *mx = (struct multiplexer_rr *)mx_a;
	int now = NEXT(mx->prev, size);
	while (arr[now] == 0 && now != mx->prev) {
		now = NEXT(now, size);
	}
	if (p != arr[now]) {
		mx->prev = PREV(now, size);
		return 0;
	} else {
		mx->prev = now;
		return 1;
	}
}

int mx_pull(struct multiplexer_abstract *mx_a, usdl_node_t **arr, int size) {
	struct multiplexer_rr *mx = (struct multiplexer_rr *)mx_a;
	int now = mx->prev;
	int old = mx->prev;
	do {
		now = NEXT(now, size);
		if (arr[now] != 0) {
			mx->prev = PREV(now, size);
			int ret = arr[now]->request_from_down(arr[now]);
			if (ret) {
				mx->prev = PREV(now, size);
				return ret;
			}
		}
	} while (now != old);
	return 0;
}

void mx_construct_multiplexer_rr(struct multiplexer_rr *mx) {
	mx->base.pull = mx_pull;
	mx->base.push = mx_push;
	mx->prev = 0;
}
