/*
 * lds.c
 *
 *  Created on: Sep 24, 2021
 *      Author: sereshotes
 */

#include "lds.h"
#include "matrix.h"


int lds_find_bound(float x[LDS_DIM], const float b[LDS_COUNT], float bound) {
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
        return -1;
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
    return 0;
}



int lds_get_ginversed(const float b[LDS_COUNT], float bound, Matrixf *out) {
    int count = 0;
    for (int i = 0; i < LDS_COUNT; i++) {
        if (b[i] > bound) {
            count++;
        }
    }
    if (count < LDS_DIM) {
        return -1;
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

    for (int i = 0; i < LDS_DIM; i++) {
        for (int j = 0; j < LDS_COUNT; j++) {
            if (b[j] > bound) {
                *matrix_at(out, i, j) = __lds_array_str[j][i];
            } else {
                *matrix_at(out, i, j) = 0;
            }
        }
    }

    Matrixf mAtA = matrix_create_static(LDS_DIM, LDS_DIM, (float*)AtA, LDS_DIM * LDS_DIM);

    matrix_inverse_and_multiplicate_left(&mAtA, out);
    return 0;
}


void lds_solution(const float b[LDS_COUNT], const Matrixf *ginv, float x[3]) {
    Matrixf mx = matrix_create_static(LDS_DIM, 1, (float*)x, LDS_DIM);
    Matrixf mb = matrix_create_static(LDS_COUNT, 1, (float*)b, LDS_COUNT);
    matrix_multiplicate(ginv, &mb, &mx);
}

float lds_calc_error(const float b[LDS_COUNT], float bound, const Matrixf *ginv, const float x[3]) {
    float sum_diff_sq = 0;
    float sum_sq = 0;
    float sumsq = 0;
        for (int i = 0; i < LDS_COUNT; i++) {
            if (b[i] > bound) {
                float b0i = 0;
                for (int j = 0; j < LDS_DIM; j++) {
                    b0i += __lds_array_str[i][j] * x[j];
                }
                float d = b0i - b[i];
                sumsq += d * d;
            }
        }
        float sumb = 0;
        for (int i = 0; i < LDS_COUNT; i++) {
            sumb += b[i] * b[i];
        }

        return sqrt(sumsq / sumb);
}

