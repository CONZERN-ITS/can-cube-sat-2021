/*
 * stack_node.hpp
 *
 *  Created on: 3 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_TC_STACK_NODE_HPP_
#define SDL_TC_STACK_NODE_HPP_

#include <inttypes.h>

template <typename params_t>
class stack_node {
private:

public:
	virtual int push(uint8_t *data, size_t size, params_t params) = 0;
	virtual bool notify(void) = 0;
};

class stack_node<void> {
private:

public:
	virtual int push(uint8_t *data, size_t size) = 0;
	virtual bool notify(void) = 0;
};



#endif /* SDL_TC_STACK_NODE_HPP_ */
