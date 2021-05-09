/*
 * ahrs.c
 *
 *  Created on: 30 мар. 2019 г.
 *      Author: sereshotes
 */

#include "madgwick.h"

#include "matrix.h"
#include "quaternion.h"

static float __arr_result_4x1[4 * 1];
static float __arr_temp_4x1[4 * 1];
static float __arr_F_3x1[3 * 1];
static float __arr_J_4x3[4 * 3];
static Matrixf __result = {
        .arr = __arr_result_4x1,
        .height = 4,
        .width = 1,
        .reserved = 4 * 1
};
static Matrixf __temp = {
        .arr = __arr_temp_4x1,
        .height = 4,
        .width = 1,
        .reserved = 4 * 1
};
static Matrixf __F = {
        .arr = __arr_F_3x1,
        .height = 3,
        .width = 1,
        .reserved = 3 * 1
};
static Matrixf __J = {
        .arr = __arr_J_4x3,
        .height = 4,
        .width = 3,
        .reserved = 4 * 3
};

static int madgwick_aproachVector(Matrixf *result, const vector_t *real, const vector_t *measured, const quaternion_t *expected);
static int madgwick_jacobianAproachVector(Matrixf *result, const vector_t *real, const quaternion_t *expected);

/*
 * Считает производную кватерниона ориентации СО Земли по отношению к СО аппарата. Эта проиводная
 * определяет скорость поворота на основании показаний гироскопа
 */
int madgwick_getGyroDerOri(quaternion_t *result, const vector_t *gyro_data, float dt, const quaternion_t *previous)
{
	quaternion_t S = vecToQuat(gyro_data);

	*result = quat_mulByQuat(previous, &S);
	*result = quat_mulByNum(result, 0.5);
	//quat_add(result, previous);

	return 0;
}

/*
 * Считает ошибку в ориентации на основании измеренных векторов и того, где они
 * должны быть в данный момент времени с учетом ориентации СО Земли относительно СО аппарата.
 * Эта ошибка может быть рассмотренна и как кватернион поворота предыдущей ориентации в текущую.
 *
 * Использует метод градиентного спуска
 */
int madgwick_getErrorOri(quaternion_t *result, const vector_t * const real[], const vector_t * const measured[], const float portions[], int count, const quaternion_t *previous)
{

	Matrixf *_result = &__result;
	matrix_make_zero(_result);
	Matrixf *temp = &__temp;
	Matrixf *F = &__F;
	Matrixf *J = &__J;


	for(int i = 0; i < count; i++)
	{
		madgwick_aproachVector(F, real[i], measured[i], previous);
		madgwick_jacobianAproachVector(J, real[i], previous);

		matrix_multiplicate(J, F, temp);
		matrix_mulNumber(temp, portions[i]);

		matrix_add(_result, temp);
		//matrix_print(&F);
		//matrix_print(&J);
	}
	float n = matrix_norm(_result);
	if(n != 0)
		matrix_mulNumber(_result, 1 / n);

	matrixToQuat(result, _result);
	return 0;
}

/*
 * Определяем расхождение между векторами (с учетом ориентации СО Земли относительно СО аппарата)
 */
static int madgwick_aproachVector(Matrixf* result, const vector_t *real, const vector_t *measured, const quaternion_t *expected)
{
	float q1 = expected->w;
	float q2 = expected->x;
	float q3 = expected->y;
	float q4 = expected->z;

	float dx = real->x;
	float dy = real->y;
	float dz = real->z;

	float sx = measured->x;
	float sy = measured->y;
	float sz = measured->z;

	quaternion_t x = quat_getConj(expected);
	quaternion_t y = vecToQuat(real);
	x = quat_mulByQuat(&x, &y);
	x = quat_mulByQuat(&x, expected);

    *matrix_at(result, 0, 0) = x.x - measured->x;
    *matrix_at(result, 1, 0) = x.y - measured->y;
    *matrix_at(result, 2, 0) = x.z - measured->z;

	//*matrix_at(result, 0, 0) = 2*dx*(0.5 - q3*q3 - q4*q4) + 2*dy*(q1*q4 + q2*q3) + 2*dz*(q2*q4 - q1*q3) - sx;
	//*matrix_at(result, 1, 0) = 2*dx*(q2*q3 - q1*q4) + 2*dy*(0.5 - q2*q2 - q4*q4) + 2*dz*(q1*q2 + q3*q4) - sy;
	//*matrix_at(result, 2, 0) = 2*dx*(q1*q3 + q2*q4) + 2*dy*(q3*q4 - q1*q2) + 2*dz*(0.5 - q2*q2 - q3*q3) - sz;
	return 0;
}

/*
 * Якобиан фукнкции madgwick_aproachVector относительно параметра madgwick_aproachVector
 */
static int madgwick_jacobianAproachVector(Matrixf *result, const vector_t *real, const quaternion_t *expected)
{
	float q1 = expected->w;
	float q2 = expected->x;
	float q3 = expected->y;
	float q4 = expected->z;

	float dx = real->x;
	float dy = real->y;
	float dz = real->z;


	*matrix_at(result, 0, 0) = 2 * (dy * q4 - dz * q3);
	*matrix_at(result, 0, 1) = 2 * (dz * q2 - dx * q4);
	*matrix_at(result, 0, 2) = 2 * (dx * q3 - dy * q2);

	*matrix_at(result, 1, 0) = 2 * (dy * q3 + dz * q4);
	*matrix_at(result, 1, 1) = 2 * (dx * q3 - 2 * dy * q2 + dz * q1);
	*matrix_at(result, 1, 2) = 2 * (dx * q4 - 2 * dz * q2 - dy * q1);

	*matrix_at(result, 2, 0) = 2 * (dy * q2 - 2 * dx * q3 - dz * q1);
	*matrix_at(result, 2, 1) = 2 * (dx * q2 + dz * q4);
	*matrix_at(result, 2, 2) = 2 * (dx * q1 - 2 * dz * q3 - dy * q4);

	*matrix_at(result, 3, 0) = 2 * (dy * q1 - 2 * dx * q4 + dz * q2);
	*matrix_at(result, 3, 1) = 2 * (dz * q3 - 2 * dy * q4 - dx * q1);
	*matrix_at(result, 3, 2) = 2 * (dx * q2 + dy * q3);
	return 0;
}

/*
 * Считает итоговую ориентацию
 * quaternion_t *error определяет скорость изменения кватерниона на основании векторов.
 * quaternion_t *gyroDerOri определяет скорость изменеия кватерниона на основании гироскопа.
 * Объединяем их с использованием доверительного коэфициента float koef_B.
 * Интегруем показания - получаем искомы кватернион
 */
int madgwick_getEstimatedOri(quaternion_t *result, const quaternion_t *gyroDerOri, const quaternion_t *error, float dt, float koef_B, const quaternion_t *previous)
{
	*result = quat_mulByNum(error, -koef_B);
	quat_add(result, gyroDerOri);
	*result = quat_mulByNum(result, dt);
	quat_add(result, previous);
	return 0;
}

