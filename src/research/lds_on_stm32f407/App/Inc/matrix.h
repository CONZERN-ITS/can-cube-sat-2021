/*
 * matrix.h
 *
 *  Created on: Mar 9, 2019
 *      Author: sereshotes
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include <stddef.h>

typedef struct
{
	int height;
	int width;
	float *arr;

	int reserved;

} Matrixf;

///////Basic
Matrixf matrix_create(int height, int width);

Matrixf matrix_create_static(int height, int width, float *arr, size_t size);

void matrix_delete(Matrixf *matrix);

void matrix_resize(Matrixf *matrix, int height, int width);

float *matrix_at(const Matrixf *matrix, int row, int column);

//void matrix_print(const Matrixf *matrix);

void matrix_setSize(Matrixf *matrix, int height, int width);

int matrix_copy(const Matrixf *source, Matrixf *destination, int isForced);

int matrix_checkSize(const Matrixf *matrix, int height, int width);

int matrix_checkSizeM(const Matrixf *a, const Matrixf *b);
///////

///////Unique matrices
void matrix_make_zero(Matrixf *matrix);

void matrix_make_identity(Matrixf *matrix);
///////

///////Matrix properties
void matrix_transpose(Matrixf* matrix);

float matrix_norm(const Matrixf *matrix);

float matrix_dot(const Matrixf *a, const Matrixf *b);
///////

///////Operations
int matrix_normalize(Matrixf *matrix);

int matrix_swapRows(Matrixf *matrix, int i1, int i2);

int matrix_mulRowNum(Matrixf *matrix, int index, float b);

int matrix_addRow(Matrixf *matrix, int source, int destination, float koef);

void matrix_mulNumber(Matrixf *matrix, float koef);

int matrix_multiplicate(const Matrixf *left, const Matrixf *right, Matrixf* result);

int matrix_add(Matrixf *dest, const Matrixf *src);

int matrix_sub(Matrixf *dest, const Matrixf *src);

int matrix_inverse_and_multiplicate_left(Matrixf *matrix, Matrixf *result);
///////


#endif /* MATRIX_H_ */
