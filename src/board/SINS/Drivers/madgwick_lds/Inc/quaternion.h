/*
 * quaternion.h
 *
 *  Created on: 01 нояб. 2017 г.
 *      Author: developer
 */

#ifndef MADGWICK_QUATERNION_H_
#define MADGWICK_QUATERNION_H_

#include "matrix.h"

typedef struct {
	double x, y, z;
} vector_t;

typedef struct {
	double w, x, y, z;
} quaternion_t;


quaternion_t quat_getConj(const quaternion_t *a);
quaternion_t quat_mulByNum(const quaternion_t * a, double k);
double quat_getNorm(const quaternion_t * a);
void quat_normalize(quaternion_t * a);
quaternion_t quat_getInverted(const quaternion_t * a);
quaternion_t quat_mulByQuat(const quaternion_t * a, const quaternion_t * b);
quaternion_t quat_mulByVec(const quaternion_t * a, const vector_t * b);
vector_t vec_rotate(const vector_t * vect, const quaternion_t * rotation);
float vec_norm(const vector_t *a);
void vec_normate(vector_t *a);

void quat_add(quaternion_t *left, const quaternion_t *right);
void quat_sub(quaternion_t *left, const quaternion_t *right);


vector_t vec_arrToVec(const float arr[3]);
int vecToMatrix(Matrixf *result, const vector_t *vec);
int quatToMatrix(Matrixf *result, const quaternion_t *quat);
quaternion_t vecToQuat(const vector_t *vec);
int matrixToQuat(quaternion_t *result, const Matrixf *m);

quaternion_t quat_zero();
quaternion_t quat_init(double w, double x, double y, double z);


vector_t vec_init(double x, double y, double z);
vector_t vec_zero();

void quat_print(const quaternion_t *a);

#endif /* MADGWICK_QUATERNION_H_ */
