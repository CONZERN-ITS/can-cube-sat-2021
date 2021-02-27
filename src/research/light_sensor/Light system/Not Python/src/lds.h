/*
 * lds.h
 *
 *  Created on: Dec 5, 2020
 *      Author: sereshotes
 */

#ifndef LDS_H_
#define LDS_H_


int lds_find_dir(int step_count, const Matrixf *sensor_array, const Matrixf *sensor_values,
		const Matrixf *x, float koef, float (*f)(float x), float (*g)(float x));



#endif /* LDS_H_ */
