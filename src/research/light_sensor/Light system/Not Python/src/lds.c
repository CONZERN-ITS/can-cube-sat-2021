/*
 * lds.c
 *
 *  Created on: Dec 5, 2020
 *      Author: sereshotes
 */

#include "matrix.h"
#include <math.h>

int _lds_one_step(const Matrixf *sensor_array, const Matrixf *sensor_values,
		const Matrixf *x0, Matrixf *x_next, float koef, float (*f)(float x), float (*g)(float x)) {

	Matrixf Jn = matrix_create(4, sensor_array->height);
	Matrixf *pJn = &Jn;
	Matrixf f_values = matrix_create(sensor_values->height, sensor_values->width);
	Matrixf *pf_values = &f_values;

	float d = *matrix_at(x0, 3, 0);
	Matrixf x = {0};
	Matrixf *px = &x;
	px->arr = x0->arr;
	px->height = 3;
	px->width = 1;

	Matrixf a;
	Matrixf *pa = &a;
	pa->height = 3;
	pa->width = 1;

	Matrixf t = matrix_create(4, 1);
	Matrixf *temp = &t;
	temp->height = 3;


	Matrixf ji = {0};
	Matrixf *pj = &ji;
	pj->height = 3;
	pj->width = 1;

	float n2 = matrix_dot(px, px);
	float n = sqrtf(n2);

	for (int i = 0; i < sensor_values->height; i++) {
		pa->arr = sensor_array->arr + i * 3;

		float na = sqrt(matrix_dot(pa, pa));

		float a_x = matrix_dot(pa, px) / na;
		if (na < 0.0001)
			a_x = 0;

		float fi = f(a_x / n) * n + d;
		float gi = g(a_x / n) * n;

		pj->arr = pJn->arr + i * 4;

		matrix_copy(pa, pj, 0);
		matrix_mulNumber(pj, gi / na);
		matrix_copy(px, temp, 0);
		matrix_mulNumber(temp, (fi - gi * a_x) / n2);
		matrix_add(pj, temp);
		pJn->arr[i * 4 + 3] = 1;

		pf_values->arr[i] = fi;
	}

	temp->height = 4;
	matrix_sub(pf_values, sensor_values);
	matrix_multiplicate(pJn, pf_values, temp);

	matrix_mulNumber(temp, koef);

	matrix_copy(x0, x_next, 0);
	matrix_sub(x_next, temp);


	matrix_delete(pJn);
	matrix_delete(pf_values);
	matrix_delete(temp);

	return 0;
}

int lds_find_dir(int step_count, const Matrixf *sensor_array, const Matrixf *sensor_values,
		Matrixf *x, float koef, float (*f)(float x), float (*g)(float x)) {
	Matrixf x0 = matrix_create(4, 1);
	for (int i = 0; i < x0.height; i++) {
		x0.arr[i] = 0;
	}
	x0.arr[0] = 1;

	for (int i = 0; i < step_count; i++) {
		matrix_print(&x0);
		_lds_one_step(sensor_array, sensor_values, &x0, x, koef, f, g);
		matrix_copy(x, &x0, 0);
	}

	return 0;
}
