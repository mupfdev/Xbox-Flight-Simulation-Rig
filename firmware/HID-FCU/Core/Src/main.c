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
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdint.h>
#include "display.h"
#include "usbd_hid.h"
#include "usb_hid_keys.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  FCU_KEY_AP,
  FCU_KEY_HDG,
  FCU_KEY_UP,
  FCU_KEY_NAV,
  FCU_KEY_ALT,
  FCU_KEY_DOWN

} fcu_key;

typedef enum
{
  MODE_AP,       /* 0 = OFF, 1 = ON  */
  MODE_NAV,      /* 0 = OFF, 1 = ON  */
  MODE_LATERAL,  /* 0 = ROL, 1 = HDG */
  MODE_VERTICAL, /* 0 = VS,  1 = ALT */

} fcu_mode;

typedef enum
{
  STATE_COLD_DARK,
  STATE_BAT_AVIONICS_ON,
  STATE_STARTER_ON,
  STATE_IGNITION_ON,
  STATE_GENERATOR_ON,
  STATE_IGNITION_OFF,
  STATE_STARTER_OFF,
  STATE_DONE

} fcu_state;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t       hid_report[8]      = { 0 };
static fcu_mode      mode               = 0;
static fcu_state     state              = STATE_COLD_DARK;
static int32_t       altitude           = 0;
static uint32_t      disp_state         = SEG_OFF;
static uint32_t      prev_disp_state    = SEG_CLR;
static bool          is_initialised         = false;
static GPIO_PinState prev_rot_1_dt = GPIO_PIN_RESET;
static GPIO_PinState prev_rot_2_dt = GPIO_PIN_RESET;
static GPIO_PinState master_bat_alt;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
void handle_encoder(void);
void handle_key(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, const fcu_key key);
void handle_switch(void);
void set_next_state(void);
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
  /* USER CODE BEGIN 2 */
  master_bat_alt = HAL_GPIO_ReadPin(MASTER_BAT_ALT_GPIO_Port, MASTER_BAT_ALT_Pin);
  prev_rot_1_dt  = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
  prev_rot_2_dt  = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (false == is_initialised)
  {
    if (master_bat_alt != HAL_GPIO_ReadPin(MASTER_BAT_ALT_GPIO_Port, MASTER_BAT_ALT_Pin))
    {
      set_next_state();
      is_initialised = true;
      master_bat_alt = HAL_GPIO_ReadPin(MASTER_BAT_ALT_GPIO_Port, MASTER_BAT_ALT_Pin);
      continue;
    }

    HAL_Delay(10);
  }

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    handle_switch();
    handle_encoder();
    handle_key(SL1_GPIO_Port, SL1_Pin, FCU_KEY_AP);
    handle_key(SL2_GPIO_Port, SL2_Pin, FCU_KEY_HDG);
    handle_key(SL3_GPIO_Port, SL3_Pin, FCU_KEY_UP);
    handle_key(SL4_GPIO_Port, SL4_Pin, FCU_KEY_NAV);
    handle_key(SL5_GPIO_Port, SL5_Pin, FCU_KEY_ALT);
    handle_key(SL6_GPIO_Port, SL6_Pin, FCU_KEY_DOWN);
    HAL_Delay(10);
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DISP_2_CLK_Pin|DISP_2_DIO_Pin|DISP_1_CLK_Pin|DISP_1_DIO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SL1_Pin SL2_Pin SL3_Pin SL4_Pin
                           SL5_Pin SL6_Pin ROT_1_CLK_Pin ROT_1_DT_Pin */
  GPIO_InitStruct.Pin = SL1_Pin|SL2_Pin|SL3_Pin|SL4_Pin
                          |SL5_Pin|SL6_Pin|ROT_1_CLK_Pin|ROT_1_DT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DISP_2_CLK_Pin DISP_2_DIO_Pin DISP_1_CLK_Pin DISP_1_DIO_Pin */
  GPIO_InitStruct.Pin = DISP_2_CLK_Pin|DISP_2_DIO_Pin|DISP_1_CLK_Pin|DISP_1_DIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT_2_SW_Pin ROT_2_CLK_Pin ROT_2_DT_Pin ROT_1_SW_Pin */
  GPIO_InitStruct.Pin = ROT_2_SW_Pin|ROT_2_CLK_Pin|ROT_2_DT_Pin|ROT_1_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MASTER_BAT_ALT_Pin */
  GPIO_InitStruct.Pin = MASTER_BAT_ALT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MASTER_BAT_ALT_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void handle_encoder(void)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  static int32_t heading_queue       = 0;
  static bool    heading_lock        = false;
  static int32_t heading_step_locked = 0;

  GPIO_PinState rot_1_dt = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
  GPIO_PinState rot_2_dt = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);

  /* Rotary Encoder 1 - ALTITUDE */
  if ((GPIO_PIN_SET == prev_rot_1_dt) && (GPIO_PIN_RESET == rot_1_dt))
  {
    if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ROT_1_CLK_GPIO_Port, ROT_1_CLK_Pin))
    {
      if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(ROT_1_SW_GPIO_Port, ROT_1_SW_Pin))
      {
        hid_report[2] = KEY_NONE;
      }
      else
      {
        hid_report[2] = KEY_MINUS;
      }
      altitude -= 100;
    }
    else
    {
      if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(ROT_1_SW_GPIO_Port, ROT_1_SW_Pin))
      {
        hid_report[2] = KEY_NONE;
      }
      else
      {
        hid_report[2] = KEY_EQUAL;
      }
      altitude += 100;
    }

    USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
    HAL_Delay(40);

    /* Release keys. */
    for (int i = 0; i < 8; i += 1)
    {
      hid_report[i] = KEY_NONE;
    }
    USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);

    set_display(disp_state, altitude);
  }

  /* Rotary Encoder 2 - HEADING */
  if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(ROT_2_SW_GPIO_Port, ROT_2_SW_Pin))
  {
    heading_lock = true;
  }

  if ((GPIO_PIN_SET == prev_rot_2_dt) && (GPIO_PIN_RESET == rot_2_dt))
  {
    if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ROT_2_CLK_GPIO_Port, ROT_2_CLK_Pin))
    {
      heading_queue -= 1;

      if (1 == heading_step_locked)
      {
        heading_lock = false;
      }
      else
      {
        heading_step_locked = -1;
      }
    }
    else
    {
      heading_queue += 1;

      if (-1 == heading_step_locked)
      {
        heading_lock = false;
      }
      else
      {
        heading_step_locked = 1;
      }
    }
  }
  else if (true == heading_lock)
  {
    heading_queue = heading_step_locked;
  }

  if (0 != heading_queue)
  {
    if (heading_queue < 0)
    {
      hid_report[2] = KEY_LEFTBRACE;
      heading_queue += 1;
    }
    else if (heading_queue > 0)
    {
      hid_report[2] = KEY_RIGHTBRACE;
      heading_queue -= 1;
    }

    USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
    HAL_Delay(40);

    /* Release keys. */
    for (int i = 0; i < 8; i += 1)
    {
      hid_report[i] = KEY_NONE;
    }
    USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
  }

  prev_rot_1_dt = rot_1_dt;
  prev_rot_2_dt = rot_2_dt;
}

void handle_key(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, const fcu_key key)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;
  static fcu_key key_lock = 0;

  if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOx, GPIO_Pin))
  {
    if (0 == ((key_lock >> key) & 1U))
    {
      switch (key)
      {
        case FCU_KEY_AP:
          hid_report[2] = KEY_TAB;

          mode ^= 1UL << MODE_AP;

          if (0 == ((mode >> MODE_AP) & 1U))
          {
            disp_state = SEG_OFF;
          }
          else if (1 == ((mode >> MODE_NAV) & 1U))
          {
            disp_state = SEG_NAV;
          }
          else if (1 == ((mode >> MODE_LATERAL) & 1U))
          {
            disp_state = SEG_HDG;
          }
          else
          {
            disp_state = SEG_ROL;
          }
          break;
        case FCU_KEY_HDG:
          hid_report[0] = KEY_MOD_LSHIFT;
          hid_report[2] = KEY_Z;

          mode ^= 1UL << MODE_LATERAL;
          mode |= 1UL << MODE_AP; /* Pilatus PC-6 (Milviz) */

          if (0 == ((mode >> MODE_AP) & 1U))
          {
            disp_state = SEG_OFF;
          }
          else if (1 == ((mode >> MODE_NAV) & 1U))
          {
            disp_state = SEG_NAV;
          }
          else if (1 == ((mode >> MODE_LATERAL) & 1U))
          {
            disp_state = SEG_HDG;
          }
          else
          {
            disp_state = SEG_ROL;
          }
          break;
        case FCU_KEY_UP:
          hid_report[0] = KEY_MOD_LALT;
          hid_report[2] = KEY_I;
          break;
        case FCU_KEY_NAV:
          hid_report[0] = KEY_MOD_LCTRL;
          hid_report[2] = KEY_Z;

          mode ^= 1UL << MODE_NAV;

          if (0 == ((mode >> MODE_AP) & 1U))
          {
            disp_state = SEG_OFF;
          }
          else if (1 == ((mode >> MODE_NAV) & 1U))
          {
            disp_state = SEG_NAV;
          }
          else if (1 == ((mode >> MODE_LATERAL) & 1U))
          {
            disp_state = SEG_HDG;
          }
          else
          {
            disp_state = SEG_ROL;
          }
          break;
        case FCU_KEY_ALT:
          hid_report[0] = KEY_MOD_LSHIFT;
          hid_report[2] = KEY_X;

          mode ^= 1UL << MODE_VERTICAL;
          mode |= 1UL << MODE_AP; /* Pilatus PC-6 (Milviz) */

          if (1 == ((mode >> MODE_AP) & 1U))
          {
            if (1 == ((mode >> MODE_VERTICAL) & 1U))
            {
              disp_state = SEG_ALT;
            }
            else
            {
              disp_state = SEG_VS;
            }
          }
          else
          {
            disp_state = SEG_OFF;
          }
          break;
        case FCU_KEY_DOWN:
          hid_report[0] = KEY_MOD_LALT;
          hid_report[2] = KEY_K;
          break;
      }

      if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ROT_1_SW_GPIO_Port, ROT_1_SW_Pin))
      {
        USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
        HAL_Delay(40);

        /* Release keys. */
        for (int i = 0; i < 8; i += 1)
        {
          hid_report[i] = KEY_NONE;
        }
        USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
      }
    }

    key_lock |= 1UL << key;
  }
  else
  {
    key_lock &= ~(1UL << key);
  }

  if ((disp_state != prev_disp_state) && (true == is_initialised))
  {
    set_display(disp_state, altitude);
    prev_disp_state = disp_state;
  }
}

void handle_switch(void)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  if (master_bat_alt != HAL_GPIO_ReadPin(MASTER_BAT_ALT_GPIO_Port, MASTER_BAT_ALT_Pin))
  {
    set_next_state();
    master_bat_alt = HAL_GPIO_ReadPin(MASTER_BAT_ALT_GPIO_Port, MASTER_BAT_ALT_Pin);
  }
}

void set_next_state(void)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  state = state + 1;

  switch (state)
  {
    default:
    case STATE_COLD_DARK:
    case STATE_DONE:
      state = STATE_DONE;
      return;
    case STATE_BAT_AVIONICS_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_B;
      hid_report[3] = KEY_A;
      break;
    case STATE_STARTER_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_Q;
      hid_report[3] = KEY_W; /* Hack: This is needed to flip the ignition switch in the next state. */
      break;
    case STATE_IGNITION_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_W;
      break;
    case STATE_GENERATOR_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_N;
      break;
    case STATE_IGNITION_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_W;
      break;
    case STATE_STARTER_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_Q;
      break;
  }

  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
  HAL_Delay(40);

  /* Release keys. */
  for (int i = 0; i < 8; i += 1)
  {
    hid_report[i] = KEY_NONE;
  }
  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);

  if (STATE_BAT_AVIONICS_ON == state)
  {
    for (int c = 1; c <= 4; c+= 1)
    {
      set_display(SEG_PFT, c);
      HAL_Delay(2000);
    }
    HAL_Delay(2000);

    fill_display();
    HAL_Delay(4000);
    set_display(SEG_OFF, 0);
  }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
