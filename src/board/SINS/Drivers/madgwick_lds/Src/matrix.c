/*
 * matrix.c
 *
 *  Created on: Mar 8, 2019
 *      Author: sereshotes
 */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"
#include <errno.h>
#include <assert.h>

#define EPS 1 / (1 << 20)
#define BAD_SIZES 0x10
#define BAD_INDEX 0x11
#define DET_IS_ZERO 0x12
#define NORM_IS_ZERO 0x13

#define RESERVE_KOEF 1.5

Matrixf matrix_create(int height, int width)
{
	Matrixf result;
	result.height = height;
	result.width = width;
	result.reserved = height * width;
	result.arr = malloc(result.reserved * sizeof(result.arr[0]));
	return result;
}

Matrixf matrix_create_static(int height, int width, float *arr, size_t size)
{
    assert(height * width <= (int)size);
    Matrixf result;
    result.height = height;
    result.width = width;
    result.reserved = size;
    result.arr = arr;
    return result;
}

void matrix_delete(Matrixf *matrix)
{
	free(matrix->arr);
	matrix->reserved = -1;
}

void matrix_resize(Matrixf *matrix, int height, int width)
{
	int r = height * width;
	if(matrix->reserved < r)
	{
		matrix->reserved = RESERVE_KOEF * r;
		matrix->arr = realloc(matrix->arr, matrix->reserved * sizeof(matrix->arr[0]));
	}
	matrix->height = height;
	matrix->width = width;
}

int matrix_isInRange(const Matrixf *matrix, int row, int column)
{
	return !(row >= matrix->height || column >= matrix->width || row < 0 || column < 0);
}

float *matrix_at(const Matrixf *matrix, int row, int column)
{
	if(!matrix_isInRange(matrix, row, column))
	{
		fprintf(stderr, "Matrix out of range!\n");
		return NULL;
	}
	return &matrix->arr[row * matrix->width + column];
}

void matrix_foreach(Matrixf *matrix, void (*fun)(Matrixf*, int, int))
{
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
			fun(matrix, i, j);
}
/*
static void _matrix_print(const Matrixf *x, int a, int b)
{
	printf("%f ", *matrix_at(x, a, b));
	if (b == x->width - 1)
		printf("\n");

}
void matrix_print(const Matrixf *matrix)
{
	matrix_foreach((Matrixf *)matrix, (void (*)(Matrixf*, int, int)) _matrix_print);
	printf("\n");
}*/


int matrix_add(Matrixf *dest, const Matrixf *src)
{
	if(dest->height != src->height || dest->width != src->width)
	{
		fprintf(stderr, "Matrix sizes do not appropriate\n");
		return BAD_SIZES;
	}
	for(int i = 0; i < dest->height; i++)
	{
		for(int j = 0; j < dest->width; j++)
		{
			*matrix_at(dest, i, j) += *matrix_at(src, i, j);
		}
	}
	return 0;
}

int matrix_sub(Matrixf *dest, const Matrixf *src)
{
	if(dest->height != src->height || dest->width != src->width)
	{
		fprintf(stderr, "Matrix sizes do not appropriate\n");
		return BAD_SIZES;
	}
	for(int i = 0; i < dest->height; i++)
	{
		for(int j = 0; j < dest->width; j++)
		{
			*matrix_at(dest, i, j) -= *matrix_at(src, i, j);
		}
	}
	return 0;
}

void matrix_make_identity(Matrixf *matrix)
{
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
		{
			if (i != j)
				*matrix_at(matrix, i, j) = 0;
			else
				*matrix_at(matrix, i, j) = 1;
		}
}

void matrix_make_zero(Matrixf *matrix)
{
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
			*matrix_at(matrix, i, j) = 0;
}


int matrix_multiplicate(const Matrixf *left, const Matrixf *right, Matrixf* result)
{
	if (left->width != right->height || right->width != result->width
			|| left->height != result->height)
	{
		fprintf(stderr, "Matrix sizes do not appropriate\n");
		return BAD_SIZES;
	}
	for (int i = 0; i < left->height; i++)
	{
		for (int j = 0; j < right->width; j++)
		{
			*matrix_at(result, i, j) = 0;
			for (int k = 0; k < left->width; k++)
			{
				*matrix_at(result, i, j) += *matrix_at(left, i, k) * *matrix_at(right, k, j);
			}
		}
	}
	return 0;
}
int matrix_swapRows(Matrixf *matrix, int i1, int i2)
{
	if (i1 >= matrix->height || i2 >= matrix->height)
	{
		fprintf(stderr, "Matrix index is out of range\n");
		return BAD_INDEX;
	}
	for(int i = 0; i < matrix->width; i++)
	{
		float t = *matrix_at(matrix, i1, i);
		*matrix_at(matrix, i1, i) = *matrix_at(matrix, i2, i);
		*matrix_at(matrix, i2, i) = t;
	}
	return 0;
}
int matrix_mulRowNum(Matrixf *matrix, int index, float b)
{
	if (index >= matrix->height)
	{
		fprintf(stderr, "Matrix index is out of range\n");
		return BAD_INDEX;
	}
	for (int i = 0; i < matrix->width; i++)
	{
		*matrix_at(matrix, index, i) *= b;
	}
	return 0;
}

int matrix_addRow(Matrixf *matrix, int source, int destination, float koef)
{
	if (!matrix_isInRange(matrix, source, 0) || !matrix_isInRange(matrix, destination, 0))
	{
		fprintf(stderr, "Matrix index is out of range\n");
		return BAD_INDEX;
	}
	for (int i = 0; i < matrix->width; i++)
	{
		*matrix_at(matrix, destination, i) += koef * *matrix_at(matrix, source, i);
	}
	return 0;
}

void matrix_transpose(Matrixf *matrix)
{
	Matrixf result;
	matrix_setSize(&result, matrix->width, matrix->height);
	float arr[matrix->height * matrix->width];
	result.arr = arr;

	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
			*matrix_at(&result, j, i) = *matrix_at(matrix, i, j);
	matrix_copy(&result, matrix, 1);
}

int matrix_inverse_and_multiplicate_left(Matrixf *matrix, Matrixf *result) {
    assert(matrix->width == matrix->height);
    assert(result->height == matrix->height);

    for (int x = 0; x < matrix->width - 1; x++)
    {
        int i1 = -1;
        for (int i = x; i < matrix->height; i++)
        {
            if (*matrix_at(matrix, i, x) > EPS || *matrix_at(matrix, i, x) < -EPS)
            {
                i1 = i;
                break;
            }
        }
        if(i1 == -1)
        {
            fprintf(stderr, "Determinant is 0\n");
            return DET_IS_ZERO;
        }
        matrix_swapRows(result, x, i1);
        matrix_swapRows(matrix, x, i1);

        float k = -1 / *matrix_at(matrix, x, x);

        for (int i = x + 1; i < matrix->height; i++)
        {
            float t = k * *matrix_at(matrix, i, x);
            matrix_addRow(matrix, x, i, t);
            matrix_addRow(result, x, i, t);
        }
    }
    for (int i = matrix->width - 1; i >= 0; i--)
    {
        matrix_mulRowNum(result, i, 1 / *matrix_at(matrix, i, i));
        *matrix_at(matrix, i, i) = 1;

        for (int j = i - 1; j >= 0; j--)
        {
            float t = -*matrix_at(matrix, j, i);
            matrix_addRow(matrix, i, j, t);
            matrix_addRow(result, i, j, t);
        }
    }
    return 0;
}

//isForced != 0 => copy size too
int matrix_copy(const Matrixf *source, Matrixf *destination, int isForced)
{
	if(isForced)
	{
		destination->height = source->height;
		destination->width = source->width;
	}
	else if(destination->height != source->height || destination->width != source->width)
	{
		fprintf(stderr, "Matrix sizes are not copied\n");
		return BAD_SIZES;
	}
	for (int i = 0; i < destination->height; i++)
		for (int j = 0; j < source->width; j++)
			*matrix_at(destination, i, j) = *matrix_at(source, i, j);
	return 0;
}


void matrix_setSize(Matrixf *matrix, int height, int width)
{
	matrix->height = height;
	matrix->width = width;
}

float matrix_dot(const Matrixf *a, const Matrixf *b) {
	assert(matrix_checkSizeM(a, b));

	float res = 0;
	int s = a->width * a->height;
	for (int i = 0; i < s; i++) {
		res += a->arr[i] * b->arr[i];
	}
	return res;
}

float matrix_norm(const Matrixf *matrix)
{
	float result = 0;
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
		{
			float t = *matrix_at(matrix, i, j);
			result += t * t;
		}
	result = sqrtf(result);
	return result;
}

int matrix_normalize(Matrixf *matrix)
{
	float norm = matrix_norm(matrix);
	if(norm == 0)
	{
		fprintf(stderr, "Can't normalize zero matrix\n");
		return NORM_IS_ZERO;
	}
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
			*matrix_at(matrix, i, j) /= norm;
	return 0;
}

void matrix_mulNumber(Matrixf *matrix, float koef)
{
	for (int i = 0; i < matrix->height; i++)
		for (int j = 0; j < matrix->width; j++)
			*matrix_at(matrix, i, j) *= koef;
}

int matrix_checkSize(const Matrixf *matrix, int height, int width)
{
	return matrix->height == height && matrix->width == width;
}
int matrix_checkSizeM(const Matrixf *a, const Matrixf *b)
{
	return a->height == b->height && a->width == b->width;
}
/*
 float matrix_det1(Matrixf a)
 {
 if(a.height != a.width)
 {
 //some error
 return nanf();
 }
 return a.arr[0][0];
 }
 float matrix_det2(Matrixf a)
 {
 if(a.height != a.width)
 {
 //some error
 return nanf();
 }
 return a.arr[0][0] * a.arr[1][1] - a.arr[0][1] * a.arr[1][0];
 }
 float det2(float a11, float a12, float a21, floa)
 float matrix_det3(Matrixf a)
 {
 if(a.height != a.width)
 {
 //some error
 return nanf();
 }
 float sum = 0;
 sum += a.arr[0][0] * a.arr[1][1] * a.arr[2][2];
 sum += a.arr[0][1] * a.arr[1][2] * a.arr[2][0];
 sum += a.arr[1][0] * a.arr[2][1] * a.arr[0][2];

 sum -= a.arr[0][2] * a.arr[1][1] * a.arr[2][0];
 sum -= a.arr[0][1] * a.arr[1][0] * a.arr[2][2];
 sum -= a.arr[0][0] * a.arr[2][1] * a.arr[1][2];
 return sum;
 }

 void matrix_inv(Matrixf a, Matrixf res)
 {

 }
 */
