/*
 * random.h
 *
 *  Created on: 10 июл. 2021 г.
 *      Author: snork
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#ifdef __cplusplus
extern "C" {
#endif

struct rgenerator;
typedef struct rgenerator rgenerator_t;

rgenerator_t * random_generator_ctor(int seed, float mean, float stddev);
void random_genetor_dtor(rgenerator_t * generator);
float random_generator_get(rgenerator_t * generator);


#ifdef __cplusplus
}
#endif

#endif /* RANDOM_H_ */
