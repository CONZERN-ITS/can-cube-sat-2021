/*
 * map_service.hpp
 *
 *  Created on: 3 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_TC_MAP_SERVICE_HPP_
#define SDL_TC_MAP_SERVICE_HPP_

#include "service_abstract.hpp"
#include "stack_node.hpp"
#include <vector>
#include <map>

class map_sdu_service {
public:

};
#define MAP_HEADER_SIZE 1


enum map_sequence_flags_t {
	FIRST = 0x00,
	CONT = 0x01,
	LAST = 0x02,
	NOSEGM = 0x03
};
struct map_params_t {
	uint8_t id;
};

template <typename T>
struct buffer {
	virtual void push(const T&);
	virtual T pop();
	void copy(const T *data, size_t size) {

	}
	virtual size_t size() const;
	virtual size_t capacity() const;
};

class map_packet_service : public stack_node<void> {
private:
	bool pop() {
		return next->push(packet, packet_size, map_id);
	}
	bool generate_packet(bool restart) {
		size_t buffer_size = right - left;
		if (restart) {
			if (buffer_size == 0) {
				return false;
			} else if (buffer_size <= map_data_size) {
				flag = NOSEGM;
			} else {
				flag = FIRST;
			}
		} else {
			if (buffer_size > map_data_size) {
				flag = CONT;
			} else if (buffer_size > 0) {
				flag = LAST;
			} else {
				return false;
			}
		}
		packet[0] = map_id || flag << 6;
		packet_size = std::min(map_data_size, buffer_size);
		memcpy(packet + 1, left, packet_size);
		left += packet_size;
		return true;
	}


public:
	typedef void (*callback_t)(void);
	callback_t ready_to_send;
	stack_node<uint8_t> *next;

	bool blocking_allowed;
	bool segmentation_allowed;

	size_t map_packet_size;
	size_t map_data_size;
	uint8_t *packet;
	size_t packet_size;
	bool is_packet_empty;

	uint8_t *buffer;
	size_t max_buffer_size;
	uint8_t *left;
	uint8_t *right;

	uint8_t map_id;
	map_sequence_flags_t flag;



	virtual bool notify() override {
		bool test = false;
		while (!is_packet_empty && pop()) {
			is_packet_empty = !generate_packet(false);
			test = true;
		}
		ready_to_send();
		return test;
	}
	virtual int push(uint8_t data[], size_t size) override {

		if (!segmentation_allowed) {
			return 0;
		}
		if (blocking_allowed) {
			return 0;
		}

		while (!is_packet_empty && pop()) {
			is_packet_empty = !generate_packet(false);
		}

		if (!is_packet_empty) {
			return 0;
		}

		memcpy(buffer, data, size);
		left = buffer;
		right = buffer + size;
		is_packet_empty = generate_packet(true);

		while (!is_packet_empty && pop()) {
			is_packet_empty = !generate_packet(false);
		}
		return size;
	}
};



#endif /* SDL_TC_MAP_SERVICE_HPP_ */
