/*
 * task_ina.h
 *
 *  Created on: Apr 25, 2020
 *      Author: sereshotes
 */

#include "main.h"

#include "assert.h"
#include "task.h"
#include "ina219_helper.h"

#ifndef INC_TASK_INA_H_
#define INC_TASK_INA_H_

#define TINA_READ_PERIOD 0
#define TINA_CALLBACK_COUNT 3
#define INA_TIMEOUT 100


extern I2C_HandleTypeDef hi2c2;
#define INA_BUS_HANDLE (&hi2c2)

/**I2C2 GPIO Configuration
PB10     ------> I2C2_SCL
PB11     ------> I2C2_SDA
*/
#define INA_SCL_GPIO_PORT GPIOB
#define INA_SCL_GPIO_PIN GPIO_PIN_10
#define INA_SDA_GPIO_PORT GPIOB
#define INA_SDA_GPIO_PIN GPIO_PIN_11

#define INA_I2C_FORCE_RESET 	__HAL_RCC_I2C2_FORCE_RESET
#define INA_I2C_RELEASE_RESET 	__HAL_RCC_I2C2_RELEASE_RESET



#if defined CUBE_1 && !defined CUBE_2
#   define TINA_COUNT 2
#elif defined CUBE_2 && !defined CUBE_1
#   define TINA_COUNT 0
#else
#   error "invalid cube definition"
#endif

typedef struct {
    float current;
    float voltage;
} tina_value_t;


int tina219_get_value(tina_value_t **arr, int **is_valid);

void tina219_add_ina_callback(void (*f)(void));

void task_ina_init(void *arg);

void task_ina_update(void *arg);

#endif /* INC_TASK_INA_H_ */
