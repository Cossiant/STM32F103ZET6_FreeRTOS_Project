/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f1xx_hal.h"

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
#define LED1_Pin GPIO_PIN_5
#define LED1_GPIO_Port GPIOE
#define Car_Motor_1IN2_Pin GPIO_PIN_13
#define Car_Motor_1IN2_GPIO_Port GPIOB
#define Car_Motor_1IN1_Pin GPIO_PIN_14
#define Car_Motor_1IN1_GPIO_Port GPIOB
#define Car_Motor_1P1_Pin GPIO_PIN_15
#define Car_Motor_1P1_GPIO_Port GPIOB
#define Car_Motor_2IN2_Pin GPIO_PIN_11
#define Car_Motor_2IN2_GPIO_Port GPIOD
#define Car_Motor_2IN1_Pin GPIO_PIN_12
#define Car_Motor_2IN1_GPIO_Port GPIOD
#define Car_Motor_2P1_Pin GPIO_PIN_13
#define Car_Motor_2P1_GPIO_Port GPIOD
#define Motor_GPIO_CH1_Pin GPIO_PIN_6
#define Motor_GPIO_CH1_GPIO_Port GPIOG
#define Motor_GPIO_CH2_Pin GPIO_PIN_7
#define Motor_GPIO_CH2_GPIO_Port GPIOG
#define Motor_GPIO_CH3_Pin GPIO_PIN_8
#define Motor_GPIO_CH3_GPIO_Port GPIOG
#define Car_Motor_3IN2_Pin GPIO_PIN_10
#define Car_Motor_3IN2_GPIO_Port GPIOC
#define Car_Motor_3IN1_Pin GPIO_PIN_11
#define Car_Motor_3IN1_GPIO_Port GPIOC
#define Car_Motor_3P1_Pin GPIO_PIN_12
#define Car_Motor_3P1_GPIO_Port GPIOC
#define Car_Motor_4IN2_Pin GPIO_PIN_10
#define Car_Motor_4IN2_GPIO_Port GPIOG
#define Car_Motor_4IN1_Pin GPIO_PIN_11
#define Car_Motor_4IN1_GPIO_Port GPIOG
#define Car_Motor_4P1_Pin GPIO_PIN_13
#define Car_Motor_4P1_GPIO_Port GPIOG
#define LED0_Pin GPIO_PIN_5
#define LED0_GPIO_Port GPIOB
#define Beep1_Pin GPIO_PIN_8
#define Beep1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
