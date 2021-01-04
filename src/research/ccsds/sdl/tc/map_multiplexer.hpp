/*
 * map_multiplexer.hpp
 *
 *  Created on: 4 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_TC_MAP_MULTIPLEXER_HPP_
#define SDL_TC_MAP_MULTIPLEXER_HPP_


#include "stack_node.hpp"
#include <list>
#include <map>
#include <cassert>


class map_multiplexer : public stack_node<uint8_t> {
private:
	std::map<uint8_t, stack_node<void>*> services;

	const size_t map_max_size;
	struct element {
		uint8_t data[map_max_size];
		size_t size;
		int priority;
		uint8_t map_id;
	};
	std::list<element> queue;


	bool add(uint8_t data[], size_t size, uint8_t map_id) {
		queue.emplace_back();
		assert(size < map_max_size);
		memcpy(queue.back().data, data, size);
		queue.back().map_id = map_id;
		queue.back().priority = 0;
		queue.back().size = size;
		return true;
	}
	bool pop(void) {
		auto first = queue.begin();
		if (first != queue.end() && next->push(first->data, first->size) > 0) {
			queue.pop_front();
			return true;
		}
		return false;

	}
public:
	stack_node<void> *next;
	bool notify() override {

		if (pop()) {
			return true;
		}

		if (services.empty()) {
			return false;
		}
		// Здесь можно оповещать сервисы по кругу,
		// обеспечивая равноправную доступность к
		// виртуальному каналу
		static auto it = services.begin();
		auto oldit = it;

		do {
			if (it == services.end()) {
				it = services.begin();
			}
			if (it->second->notify()) {
				break;
			}
			it++;
		} while (it != oldit);

		if (it == oldit) {
			return false;
		} else {
			return pop();
		}
	}
	int push(uint8_t data[], size_t size, uint8_t map_id) override {
		pop();
		add(data, size, map_id);
		pop();
	}

};


#endif /* SDL_TC_MAP_MULTIPLEXER_HPP_ */
