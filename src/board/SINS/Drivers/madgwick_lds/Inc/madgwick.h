/*
 * ahrs.h
 *
 *  Created on: 31 мар. 2019 г.
 *      Author: sereshotes
 */

#ifndef MADGWICK_H_
#define MADGWICK_H_

#include "matrix.h"
#include "quaternion.h"

//Calculates noise error in orientation based on vector approaching (see gradient method)
int madgwick_getErrorOri(quaternion_t *result, const vector_t * const real[], const vector_t * const measured[], const float portions[], int count, const quaternion_t *previous);

//Calculates resulted orientation
int madgwick_getEstimatedOri(quaternion_t *result, const quaternion_t *gyroDerOri, const quaternion_t *error, float dt, float koef_B, const quaternion_t *previous);

//Calculates derivative of orientation based on gyro data without bias drift
int madgwick_getGyroDerOri(quaternion_t *result, const vector_t *gyro_data, float dt, const quaternion_t *previous);
#endif /* MADGWICK_H_ */
