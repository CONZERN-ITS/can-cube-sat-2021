/*
 * lds.c
 *
 *  Created on: 16 мар. 2019 г.
 *      Author: sereshotes
 */


#include <assert.h>
#include <lds.h>
#include <math.h>
#include <stdlib.h>
/*
 * Каждая из матриц должна быть проинициализирована и lds->A должна быть долполнительно
 * проинициализирована конкретными значениями
 */
void lds_init(lds_t *lds, int n, int k, Matrixf *temp) {
    assert(matrix_checkSize(&lds->A, n, k));
    assert(matrix_checkSize(&lds->As, k, n));
    assert(matrix_checkSize(temp, k, k));

    lds->n = n;
    lds->k = k;
    //Считаем A_t * A
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < k; j++) {
            for (int p = 0; p < n; p++) {
                *matrix_at(temp, i, j) = *matrix_at(&lds->A, p, i) * *matrix_at(&lds->A, p, j);
            }
        }
    }
    //A_t
    matrix_copy(&lds->A, &lds->As, 0);
    matrix_transpose(&lds->As);

    //((A_t*A)^-1)*A_t
    matrix_inverse_and_multiplicate_left(temp, &lds->As);
}

void lds_search(const lds_t *lds, const Matrixf *b, Matrixf *x) {
    assert(b->width == 1 && b->height == lds->n);
    assert(x->width == 1 && x->height == lds->k);
    //x = ((A_t*A)^-1)*A_t*b
    matrix_multiplicate(&lds->As, b, x);
}
//Возвращает стандартное отклонение
float lds_get_error(const lds_t *lds, const Matrixf *b, const Matrixf *x) {
    assert(b->width == 1 && b->height == lds->n);
    assert(x->width == 1 && x->height == lds->k);
    int sumsq = 0;
    for (int i = 0; i < lds->n; i++) {
        float b0i = 0;
        for (int j = 0; j < lds->k; j++) {
            b0i += *matrix_at(&lds->A, i, j) * *matrix_at(x, j, 0);
        }
        float d = b0i - *matrix_at(b, i, 0);
        sumsq += d * d;
    }
    return sqrt(sumsq);
}
