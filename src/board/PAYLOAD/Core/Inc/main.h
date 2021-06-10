/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_rtc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"

#include "stm32f4xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BME_EXT_PWR_Pin GPIO_PIN_0
#define BME_EXT_PWR_GPIO_Port GPIOC
#define MS_EXT_PWR_Pin GPIO_PIN_1
#define MS_EXT_PWR_GPIO_Port GPIOC
#define DOSIM_PWR_Pin GPIO_PIN_2
#define DOSIM_PWR_GPIO_Port GPIOC
#define HEATER_PWR_Pin GPIO_PIN_3
#define HEATER_PWR_GPIO_Port GPIOC
#define SENS_CTRL_NO2_2_Pin GPIO_PIN_6
#define SENS_CTRL_NO2_2_GPIO_Port GPIOA
#define SENS_CTRL_NO2_1_Pin GPIO_PIN_7
#define SENS_CTRL_NO2_1_GPIO_Port GPIOA
#define MS_INT_PWR_Pin GPIO_PIN_0
#define MS_INT_PWR_GPIO_Port GPIOB
#define BME_INT_PWR_Pin GPIO_PIN_1
#define BME_INT_PWR_GPIO_Port GPIOB
#define SENS_CTRL_NH3_1_Pin GPIO_PIN_12
#define SENS_CTRL_NH3_1_GPIO_Port GPIOB
#define SENS_CTRL_NH3_2_Pin GPIO_PIN_13
#define SENS_CTRL_NH3_2_GPIO_Port GPIOB
#define SENS_CTRL_CO_1_Pin GPIO_PIN_14
#define SENS_CTRL_CO_1_GPIO_Port GPIOB
#define PPS_Input_debug_Pin GPIO_PIN_8
#define PPS_Input_debug_GPIO_Port GPIOC
#define PPS_Input_debug_EXTI_IRQn EXTI9_5_IRQn
#define SENS_CTRL_CO_2_Pin GPIO_PIN_9
#define SENS_CTRL_CO_2_GPIO_Port GPIOA
#define MICS_PWR_Pin GPIO_PIN_11
#define MICS_PWR_GPIO_Port GPIOA
#define O2_PWR_Pin GPIO_PIN_12
#define O2_PWR_GPIO_Port GPIOA
#define PPS_Input_Pin GPIO_PIN_15
#define PPS_Input_GPIO_Port GPIOA
#define PPS_Input_EXTI_IRQn EXTI15_10_IRQn
#define LED_Pin_Pin GPIO_PIN_12
#define LED_Pin_GPIO_Port GPIOC
#define COMPR_ON_Pin GPIO_PIN_5
#define COMPR_ON_GPIO_Port GPIOB
#define VALVE_ON_Pin GPIO_PIN_8
#define VALVE_ON_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define I2C_INT_Pin GPIO_PIN_7
#define I2C_INT_GPIO_Port GPIOF

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
