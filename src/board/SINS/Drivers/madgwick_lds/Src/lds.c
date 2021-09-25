/*
 * lds.c
 *
 *  Created on: Sep 24, 2021
 *      Author: sereshotes
 */

#include "lds.h"
#include "matrix.h"


void lds_find_bound(float x[LDS_DIM], const float b[LDS_COUNT], float bound) {
    int count = 0;
    for (int i = 0; i < LDS_COUNT; i++) {
        if (b[i] > bound) {
            count++;
        }
    }
    if (count < LDS_DIM) {
        x[0] = 1;
        x[1] = 0;
        x[2] = 0;
        return;
    }

    float AtA[LDS_DIM][LDS_DIM] = {0};
    for (int i = 0; i < LDS_DIM; i++) {
        for (int j = 0; j < LDS_DIM; j++) {
            for (int k = 0; k < LDS_COUNT; k++) {
                if (b[k] > bound) {
                    AtA[i][j] += __lds_array_str[k][i] * __lds_array_str[k][j];
                }
            }
        }
    }

    float bb[LDS_DIM] = {0};
    for (int i = 0; i < LDS_DIM; i++) {
        for (int j = 0; j < LDS_COUNT; j++) {
            if (b[j] > bound) {
                bb[i] += __lds_array_str[j][i] * b[j];
            }
        }
    }

    Matrixf mAtA = matrix_create_static(LDS_DIM, LDS_DIM, (float*)AtA, LDS_DIM * LDS_DIM);
    Matrixf mAtb = matrix_create_static(LDS_DIM, 1, (float*)bb, LDS_DIM);

    matrix_inverse_and_multiplicate_left(&mAtA, &mAtb);

    for (int i = 0; i < LDS_DIM; i++) {
        x[i] = bb[i];
    }
}

