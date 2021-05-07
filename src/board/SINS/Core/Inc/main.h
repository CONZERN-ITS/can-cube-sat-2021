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
#define ADC_TIMEOUT 100
#define LDiod7_Pin GPIO_PIN_0
#define LDiod7_GPIO_Port GPIOC
#define LDiod8_Pin GPIO_PIN_1
#define LDiod8_GPIO_Port GPIOC
#define LDiod4_Pin GPIO_PIN_2
#define LDiod4_GPIO_Port GPIOC
#define LDiod1_Pin GPIO_PIN_1
#define LDiod1_GPIO_Port GPIOA
#define GPS_UART_TX_Pin GPIO_PIN_2
#define GPS_UART_TX_GPIO_Port GPIOA
#define GPS_UART_RX_Pin GPIO_PIN_3
#define GPS_UART_RX_GPIO_Port GPIOA
#define LDiod2_Pin GPIO_PIN_4
#define LDiod2_GPIO_Port GPIOA
#define LDiod3_Pin GPIO_PIN_5
#define LDiod3_GPIO_Port GPIOA
#define LDiod9_Pin GPIO_PIN_4
#define LDiod9_GPIO_Port GPIOC
#define LDiod10_Pin GPIO_PIN_5
#define LDiod10_GPIO_Port GPIOC
#define LDiod5_Pin GPIO_PIN_0
#define LDiod5_GPIO_Port GPIOB
#define LDiod6_Pin GPIO_PIN_1
#define LDiod6_GPIO_Port GPIOB
#define PWR_MEMS_Pin GPIO_PIN_11
#define PWR_MEMS_GPIO_Port GPIOA
#define PWR_GPS_Pin GPIO_PIN_12
#define PWR_GPS_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

#define I2C_INT_Pin GPIO_PIN_7
#define I2C_INT_GPIO_Port GPIOF

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
