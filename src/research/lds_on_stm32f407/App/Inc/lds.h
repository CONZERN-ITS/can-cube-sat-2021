/*
 * lds.h
 *
 *  Created on: 16 мар. 2019 г.
 *      Author: sereshotes
 */

#ifndef LDS_H_
#define LDS_H_

#include <inttypes.h>

#include "matrix.h"

typedef struct lds_t {
    Matrixf A;
    Matrixf As;
    int n;
    int k;
} lds_t;

void lds_init(lds_t *lds, int n, int k, Matrixf *temp);

void lds_search(const lds_t *lds, const Matrixf *b, Matrixf *x);

float lds_get_error(const lds_t *lds, const Matrixf *b, const Matrixf *x);

#endif /* LDS_H_ */
