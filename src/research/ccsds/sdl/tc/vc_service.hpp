/*
 * virtual_service.hpp
 *
 *  Created on: 4 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_TC_VC_SERVICE_HPP_
#define SDL_TC_VC_SERVICE_HPP_



#include "stack_node.hpp"


class vc_packet_service : public stack_node<void> {
private:
	stack_node<void> *prev;
	stack_node<uint8_t> *next;

	bool pop() {
	}
	bool generate_packet(bool restart) {
	}
public:

	virtual bool notify() override {
	}
	virtual int push(uint8_t data[], size_t size) override {

	}

};


#endif /* SDL_TC_VC_SERVICE_HPP_ */
