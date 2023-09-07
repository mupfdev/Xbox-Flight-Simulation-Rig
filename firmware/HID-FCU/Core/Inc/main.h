/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include <stdint.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum
{
  STATE_OFF,
  STATE_AVIONICS_BUS2_BOOTUP,
  STATE_READY

} fcu_state;
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
#define SL1_Pin GPIO_PIN_1
#define SL1_GPIO_Port GPIOA
#define SL2_Pin GPIO_PIN_2
#define SL2_GPIO_Port GPIOA
#define SL3_Pin GPIO_PIN_3
#define SL3_GPIO_Port GPIOA
#define SL4_Pin GPIO_PIN_4
#define SL4_GPIO_Port GPIOA
#define SL5_Pin GPIO_PIN_5
#define SL5_GPIO_Port GPIOA
#define SL6_Pin GPIO_PIN_6
#define SL6_GPIO_Port GPIOA
#define DISP_2_CLK_Pin GPIO_PIN_0
#define DISP_2_CLK_GPIO_Port GPIOB
#define DISP_2_DIO_Pin GPIO_PIN_1
#define DISP_2_DIO_GPIO_Port GPIOB
#define DISP_1_CLK_Pin GPIO_PIN_10
#define DISP_1_CLK_GPIO_Port GPIOB
#define DISP_1_DIO_Pin GPIO_PIN_11
#define DISP_1_DIO_GPIO_Port GPIOB
#define ROT_2_SW_Pin GPIO_PIN_12
#define ROT_2_SW_GPIO_Port GPIOB
#define ROT_2_CLK_Pin GPIO_PIN_13
#define ROT_2_CLK_GPIO_Port GPIOB
#define ROT_2_DT_Pin GPIO_PIN_14
#define ROT_2_DT_GPIO_Port GPIOB
#define ROT_1_SW_Pin GPIO_PIN_15
#define ROT_1_SW_GPIO_Port GPIOB
#define ROT_1_CLK_Pin GPIO_PIN_8
#define ROT_1_CLK_GPIO_Port GPIOA
#define ROT_1_DT_Pin GPIO_PIN_9
#define ROT_1_DT_GPIO_Port GPIOA
#define START_SWITCH_Pin GPIO_PIN_10
#define START_SWITCH_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
