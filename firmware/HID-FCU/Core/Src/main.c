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
#include <string.h>
#include "ads1115.h"
#include "display.h"
#include "switch-panel.h"
#include "usb_hid_keys.h"
#include "usbd_hid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  AXIS_PROP = 0,
  AXIS_THROTTLE,
  AXIS_MIXTURE

} axis_index_t;

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

#define HID_FIFO_QUEUE_SIZE    100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */
static uint8_t       hid_report_fifo_queue_index = 0;
static uint8_t       hid_report_fifo_queue[HID_FIFO_QUEUE_SIZE][8] = {{ 0x00 }};

static fcu_mode      mode            = 0;
static int32_t       altitude        = 0;
static uint16_t      axis_data[3]    = { 0 };
static uint32_t      disp_state      = SEG_OFF;
static uint32_t      prev_disp_state = SEG_CLR;
static bool          is_initialised  = false;
static GPIO_PinState prev_rot_1_dt   = GPIO_PIN_RESET;
static GPIO_PinState prev_rot_2_dt   = GPIO_PIN_RESET;
static GPIO_PinState start_switch;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
static void add_report_to_fifo_queue(uint8_t hid_report[]);
static void handle_encoder(void);
static void handle_key(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, const fcu_key key);
static void handle_red_switch(void);
static void handle_levers(void);
static void send_report(uint8_t hid_report[]);
static void send_fifo_queue_item(void);
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
  fcu_state state        = STATE_OFF;
  uint32_t  bootup_delay = 10;
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
  MX_I2C1_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  ads1115_init();
  memset(hid_report_fifo_queue, 0, sizeof(hid_report_fifo_queue));

  start_switch  = HAL_GPIO_ReadPin(START_SWITCH_GPIO_Port, START_SWITCH_Pin);
  prev_rot_1_dt = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
  prev_rot_2_dt = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t battery_state;
    uint32_t av_bus2_state;
    uint32_t switch_state;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    switch (state)
    {
      case STATE_OFF:
      {
        is_initialised = false;
        handle_switch_panel();
        handle_red_switch();

        switch_state  = get_switch_state();
        battery_state = (switch_state & (1U << SW_BATTERY));
        av_bus2_state = (switch_state & (1U << SW_AVIONIC_BUS2));

        if (0 == (switch_state & (1U << SW_SYNC_MODE)))
        {
          bootup_delay = 0;
        }

        if (0 == battery_state && 0 == av_bus2_state)
        {
          state = STATE_AVIONICS_BUS2_BOOTUP;
        }
        break;
      }
      case STATE_AVIONICS_BUS2_BOOTUP:
      {
        for (int c = 1; c <= 4; c+= 1)
        {
          set_display(SEG_PFT, c);
          for (int t = 0; t < 180; t+= 1)
          {
            handle_switch_panel();
            handle_red_switch();

            switch_state  = get_switch_state();
            battery_state = (switch_state & (1U << SW_BATTERY));
            av_bus2_state = (switch_state & (1U << SW_AVIONIC_BUS2));

            if (battery_state > 0 || av_bus2_state > 0)
            {
              state = STATE_OFF;
              clear_display();
              break;
            }

            HAL_Delay(bootup_delay);
          }
        }
        for (int t = 0; t < 200; t+= 1)
        {
          handle_switch_panel();
          handle_red_switch();

          switch_state  = get_switch_state();
          battery_state = (switch_state & (1U << SW_BATTERY));
          av_bus2_state = (switch_state & (1U << SW_AVIONIC_BUS2));

          if (battery_state > 0 || av_bus2_state > 0)
          {
            state = STATE_OFF;
            clear_display();
            break;
          }

          HAL_Delay(bootup_delay);
        }

        fill_display();

        for (int t = 0; t < 400; t+= 1)
        {
          handle_switch_panel();
          handle_red_switch();

          switch_state  = get_switch_state();
          battery_state = (switch_state & (1U << SW_BATTERY));
          av_bus2_state = (switch_state & (1U << SW_AVIONIC_BUS2));

          if (battery_state > 0 || av_bus2_state > 0)
          {
            state = STATE_OFF;
            clear_display();
            break;
          }

          HAL_Delay(bootup_delay);
        }

        mode &= ~(1UL << MODE_AP);
        state = STATE_READY;

        clear_display();
        prev_disp_state = ! disp_state; /* Hack. */

        break;
      }
      case STATE_READY:
      {
        is_initialised = true;
        handle_switch_panel();
        handle_red_switch();
        handle_levers();

        switch_state  = get_switch_state();
        battery_state = (switch_state & (1U << SW_BATTERY));
        av_bus2_state = (switch_state & (1U << SW_AVIONIC_BUS2));

        if (battery_state > 0 || av_bus2_state > 0)
        {
          state = STATE_OFF;
          clear_display();
        }
        else
        {
          handle_encoder();
          handle_key(SL1_GPIO_Port, SL1_Pin, FCU_KEY_AP);
          handle_key(SL2_GPIO_Port, SL2_Pin, FCU_KEY_HDG);
          handle_key(SL3_GPIO_Port, SL3_Pin, FCU_KEY_UP);
          handle_key(SL4_GPIO_Port, SL4_Pin, FCU_KEY_NAV);
          handle_key(SL5_GPIO_Port, SL5_Pin, FCU_KEY_ALT);
          handle_key(SL6_GPIO_Port, SL6_Pin, FCU_KEY_DOWN);
        }
        break;
      }
    }

    send_fifo_queue_item();
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 64;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DISP_2_CLK_Pin|DISP_2_DIO_Pin|DISP_1_DIO_Pin|DISP_1_CLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SL1_Pin SL2_Pin SL3_Pin SL4_Pin
                           SL5_Pin SL6_Pin ROT_1_CLK_Pin ROT_1_DT_Pin */
  GPIO_InitStruct.Pin = SL1_Pin|SL2_Pin|SL3_Pin|SL4_Pin
                          |SL5_Pin|SL6_Pin|ROT_1_CLK_Pin|ROT_1_DT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DISP_2_CLK_Pin DISP_2_DIO_Pin DISP_1_DIO_Pin DISP_1_CLK_Pin */
  GPIO_InitStruct.Pin = DISP_2_CLK_Pin|DISP_2_DIO_Pin|DISP_1_DIO_Pin|DISP_1_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT_2_SW_Pin ROT_2_CLK_Pin ROT_2_DT_Pin ROT_1_SW_Pin */
  GPIO_InitStruct.Pin = ROT_2_SW_Pin|ROT_2_CLK_Pin|ROT_2_DT_Pin|ROT_1_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : START_SWITCH_Pin */
  GPIO_InitStruct.Pin = START_SWITCH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(START_SWITCH_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void add_report_to_fifo_queue(uint8_t hid_report[])
{
  if (hid_report_fifo_queue_index > HID_FIFO_QUEUE_SIZE)
  {
    hid_report_fifo_queue_index = 0;
  }

  for (int i = 0; i < 8; i += 1)
  {
    hid_report_fifo_queue[hid_report_fifo_queue_index][i] = hid_report[i];
  }

  hid_report_fifo_queue_index += 1;
}

static void handle_encoder(void)
{
  static int32_t heading_queue       = 0;
  static bool    heading_lock        = false;
  static int32_t heading_step_locked = 0;

  uint8_t hid_report[8] = { 0 };

  GPIO_PinState rot_1_dt = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
  GPIO_PinState rot_2_dt = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);

  /* Rotary Encoder 1 - ALTITUDE */
  if ((GPIO_PIN_SET == prev_rot_1_dt) && (GPIO_PIN_RESET == rot_1_dt))
  {
    uint32_t sync_mode_state = (get_switch_state() & (1U << SW_SYNC_MODE));

    if (GPIO_PIN_SET == HAL_GPIO_ReadPin(ROT_1_CLK_GPIO_Port, ROT_1_CLK_Pin))
    {
      if (sync_mode_state != 0)
      {
        hid_report[2] = KEY_MINUS;
      }
      else
      {
        hid_report[2] = KEY_NONE;
      }

      altitude -= 100;
    }
    else
    {
      if (sync_mode_state != 0)
      {
        hid_report[2] = KEY_EQUAL;
      }
      else
      {
        hid_report[2] = KEY_NONE;
      }

      altitude += 100;
    }

    add_report_to_fifo_queue(hid_report);
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

    add_report_to_fifo_queue(hid_report);
  }

  prev_rot_1_dt = rot_1_dt;
  prev_rot_2_dt = rot_2_dt;
}

static void handle_key(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, const fcu_key key)
{
  static fcu_key key_lock = 0;

  uint8_t  hid_report[8] = { 0 };
  uint32_t sync_mode_state;;

  if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOx, GPIO_Pin))
  {
    if (0 == ((key_lock >> key) & 1U))
    {
      switch (key)
      {
        case FCU_KEY_AP:
          hid_report[0] = KEY_MOD_LALT;
          hid_report[2] = KEY_V;

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
          hid_report[0] = KEY_MOD_LALT;
          hid_report[2] = KEY_B;

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
          hid_report[2] = KEY_N;
          break;
        case FCU_KEY_NAV:
          hid_report[0] = KEY_MOD_RALT;
          hid_report[2] = KEY_V;

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
          hid_report[0] = KEY_MOD_RALT;
          hid_report[2] = KEY_B;

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
          hid_report[0] = KEY_MOD_RALT;
          hid_report[2] = KEY_N;
          break;
      }

      sync_mode_state = (get_switch_state() & (1U << SW_SYNC_MODE));
      if (sync_mode_state != 0)
      {
        add_report_to_fifo_queue(hid_report);
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

static void handle_red_switch(void)
{
  uint8_t hid_report[8] = { 0 };

  if (start_switch != HAL_GPIO_ReadPin(START_SWITCH_GPIO_Port, START_SWITCH_Pin))
  {
    hid_report[0] = KEY_MOD_LALT;
    hid_report[2] = KEY_M;

    add_report_to_fifo_queue(hid_report);

    start_switch = HAL_GPIO_ReadPin(START_SWITCH_GPIO_Port, START_SWITCH_Pin);
  }
}

static void handle_levers(void)
{
  uint8_t hid_report[8] = { 0 };

  for (axis_index_t index = 0; index <= AXIS_MIXTURE; index += 1)
  {
    static int64_t prev_percentage[3] = { 50, 50, 50 };

    uint8_t channel;
    int16_t lower_limit;
    int16_t center;
    int16_t upper_limit;
    int64_t percentage[3];

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
    }
    else
    {
      percentage[index] = (100ULL * axis_data[index] + (upper_limit - lower_limit) / 2) / (upper_limit - lower_limit);

      if (percentage[index] < 51)
      {
        percentage[index] = 51;
      }
    }

    if (percentage[index] > 100)
    {
      percentage[index] = 100;
    }
    else if (percentage[index] < 0)
    {
      percentage[index] = 0;
    }

    hid_report[0] = KEY_MOD_LCTRL;

    if (((abs(prev_percentage[index] - percentage[index]) >= AXIS_THROTTLE) && (index == AXIS_THROTTLE)) ||
        ((percentage[index] == 0) && (index == AXIS_THROTTLE)))
    {
      /* Nothing to do here. */
    }
    else
    {
      prev_percentage[index] = percentage[index];
      continue;
    }

    switch (index)
    {
      case AXIS_PROP:
        break;
      case AXIS_THROTTLE:

        switch (percentage[index])
        {
          case 36:
          case 37:
          case 38:
          case 39:
          case 40:
            hid_report[2] = KEY_SYSRQ;
            break;
          case 45:
            hid_report[2] = KEY_F1;
            break;
          case 50:
            hid_report[2] = KEY_F2;
            break;
          case 55:
            hid_report[2] = KEY_F3;
            break;
          case 60:
            hid_report[2] = KEY_F4;
            break;
          case 65:
            hid_report[2] = KEY_F5;
            break;
          case 70:
            hid_report[2] = KEY_F6;
            break;
          case 75:
            hid_report[2] = KEY_F7;
            break;
          case 80:
            hid_report[2] = KEY_F8;
            break;
          case 90:
            hid_report[2] = KEY_F9;
          case 100:
            hid_report[2] = KEY_F10;
            break;
          default:
            if (prev_percentage[index] < percentage[index])
            {
              hid_report[2] = KEY_F12;
            }
            else if ((prev_percentage[index] > percentage[index]))
            {
              hid_report[2] = KEY_F11;
            }
            else
            {
              prev_percentage[index] = percentage[index];
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
            continue;
          }
        }
        break;
    }

#if 0
    if (((abs(prev_percentage[index] - percentage[index]) >= 1) && (index == 1)) ||
        ((percentage[index] == 0) && (index == 1)))
    {
#endif
      add_report_to_fifo_queue(hid_report);
#if 0
    }
#endif

    prev_percentage[index] = percentage[index];
  }
}

static void send_report(uint8_t hid_report[])
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
  HAL_Delay(40);

  /* Release keys. */
  for (int i = 0; i < 8; i += 1)
  {
    hid_report[i] = KEY_NONE;
  }
  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);

  HAL_Delay(10);
}

static void send_fifo_queue_item(void)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  uint8_t hid_report[8];
  int     first_valid_item;
  bool    valid_item_found = false;

  for (first_valid_item = 0; first_valid_item < HID_FIFO_QUEUE_SIZE; first_valid_item += 1)
  {
    for (int i = 0; i < 8; i += 1)
    {
      hid_report[i] = hid_report_fifo_queue[first_valid_item][i];
      if (hid_report[i] != 0x00)
      {
        valid_item_found = true;
      }
    }

    if (true == valid_item_found)
    {
      break;
    }
  }

  if (true == valid_item_found)
  {
    send_report(hid_report);

    /* Clear last valid item. */
    for (int i = 0; i < 8; i += 1)
    {
      hid_report_fifo_queue[first_valid_item][i] = 0x00;
    }
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
