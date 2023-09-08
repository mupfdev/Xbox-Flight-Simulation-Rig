/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include <ads1115.h>
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "usbd_hid.h"
#include "usb_hid_keys.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  AXIS_PROP = 0,
  AXIS_THROTTLE,
  AXIS_MIXTURE

} axis_index_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PROP_LOWER_LIMIT       600
#define PROP_CENTER          14914
#define PROP_UPPER_LIMIT     26300

#define THROTTLE_LOWER_LIMIT    10
#define THROTTLE_CENTER      11435
#define THROTTLE_UPPER_LIMIT 25300

#define MIXTURE_LOWER_LIMIT     10
#define MIXTURE_CENTER       13740
#define MIXTURE_UPPER_LIMIT  26300
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */
static uint16_t axis_data[3] = { 0 };
/* USER CODE END PV */

/* Private function pro
 * totypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  extern USBD_HandleTypeDef hUsbDeviceFS;
  uint8_t hid_report[8] = { 0 };
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  ads1115_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    for (axis_index_t index = 0; index <= AXIS_MIXTURE; index += 1)
    {
      static uint64_t prev_percentage[3] = { 50, 50, 50 };

      uint8_t  channel;
      int16_t  lower_limit;
      int16_t  center;
      int16_t  upper_limit;
      uint64_t percentage[3];

      switch (index)
      {
        case AXIS_PROP:
          channel     = AXIS_THROTTLE;
          lower_limit = PROP_LOWER_LIMIT;
          center      = PROP_CENTER;
          upper_limit = PROP_UPPER_LIMIT;
          break;
        case AXIS_THROTTLE:
          channel     = AXIS_MIXTURE;
          lower_limit = THROTTLE_LOWER_LIMIT;
          center      = THROTTLE_CENTER;
          upper_limit = THROTTLE_UPPER_LIMIT;
          break;
        case AXIS_MIXTURE:
          channel     = AXIS_PROP;
          lower_limit = MIXTURE_LOWER_LIMIT;
          center      = MIXTURE_CENTER;
          upper_limit = MIXTURE_UPPER_LIMIT;
          break;
      }

      ads1115_read((int16_t*)&axis_data[index], lower_limit, upper_limit);
      ads1115_set_channel(channel);

      if (axis_data[index] <= center)
      {
        percentage[index] = (50ULL * axis_data[index] + (center - lower_limit) / 2) / (center - lower_limit);

        if (percentage[index] > 50)
        {
          percentage[index] = 50;
        }
        else if (percentage[index] < 0)
        {
          percentage[index] = 0;
        }
      }
      else
      {
        percentage[index] = (100ULL * axis_data[index] + (upper_limit - lower_limit) / 2) / (upper_limit - lower_limit);

        if (percentage[index] > 100)
        {
          percentage[index] = 100;
        }
        else if (percentage[index] < 51)
        {
          percentage[index] = 51;
        }
      }

      hid_report[0] = KEY_MOD_LCTRL;

      switch (index)
      {
        case AXIS_PROP:
          break;
        case AXIS_THROTTLE:
          hid_report[2] = KEY_T;

          switch (percentage[index])
          {
            case 36:
            case 37:
            case 38:
            case 39:
            case 40:
              hid_report[3] = KEY_0;
              break;
            case 45:
              hid_report[3] = KEY_1;
              break;
            case 50:
                hid_report[3] = KEY_2;
              break;
            case 55:
              hid_report[3] = KEY_3;
              break;
            case 60:
              hid_report[3] = KEY_4;
              break;
            case 65:
              hid_report[3] = KEY_5;
              break;
            case 70:
              hid_report[3] = KEY_6;
              break;
            case 75:
              hid_report[3] = KEY_7;
              break;
            case 80:
              hid_report[3] = KEY_8;
              break;
            case 90:
              hid_report[3] = KEY_9;
            case 100:
              hid_report[3] = KEY_EQUAL;
              break;
            default:
              if ((prev_percentage[index] < percentage[index]))
              {
                hid_report[3] = KEY_RIGHTBRACE;
              }
              else if ((prev_percentage[index] > percentage[index]) || (0 == prev_percentage[index] && 0 == percentage[index]))
              {
                hid_report[3] = KEY_LEFTBRACE;
              }
              else
              {
                prev_percentage[index] = percentage[index];
                HAL_Delay(10);
                continue;
              }
              break;
          }
          break;

        case AXIS_MIXTURE:
          hid_report[2] = KEY_M;

          if (percentage[index] == 0) /* CUT OFF. */
          {
            hid_report[3] = KEY_0;
          }
          else if (prev_percentage[index] == 0) /* Transition from CUT OFF. */
          {
            if (percentage[index] >= 70) /* HIGH IDLE. */
            {
              hid_report[3] = KEY_1;
            }
            else if (percentage[index] > 0) /* LOW IDLE. */
            {
              hid_report[3] = KEY_RIGHTBRACE;
            }
          }
          else
          {
            if ((percentage[index] >= 70)) /* HIGH IDLE. */
            {
              hid_report[3] = KEY_1;
            }
            else if ((percentage[index] <= 30) && (percentage[index] != 0))
            {
              hid_report[3] = KEY_LEFTBRACE; /* LOW IDLE. */
            }
            else
            {
              prev_percentage[index] = percentage[index];
              HAL_Delay(10);
              continue;
            }
          }
          break;
      }

      if ((index == 1) ||
          ((index == 2) && ((abs(prev_percentage[index] - percentage[index]) >= 1))))
      {
        USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
        HAL_Delay(20);

        /* Release keys. */
        for (int i = 0; i < 8; i += 1)
        {
          hid_report[i] = KEY_NONE;
        }
        USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
      }

      prev_percentage[index] = percentage[index];
      HAL_Delay(10);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
