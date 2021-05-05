/*
 * matrix_static_alloc.h
 *
 *  Created on: Mar 13, 2021
 *      Author: sereshotes
 */

#ifndef INC_MATRIX_STATIC_ALLOC_H_
#define INC_MATRIX_STATIC_ALLOC_H_

#include "matrix.h"

#define MATRIX_SA(name, h, w) do {\
        static float arr[h * w];\
        name.arr = arr;\
        name.height = h;\
        name.width = w;\
    } while (0)


#endif /* INC_MATRIX_STATIC_ALLOC_H_ */
