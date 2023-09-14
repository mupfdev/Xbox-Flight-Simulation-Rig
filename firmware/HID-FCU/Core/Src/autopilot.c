/*
 * autopilot.c
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdbool.h>
#include <stdint.h>
#include "autopilot.h"
#include "cmsis_os.h"
#include "display.h"
#include "levers.h"
#include "main.h"
#include "state_handler.h"
#include "stm32f1xx_hal.h"
#include "switch_panel.h"
#include "usb_hid_handler.h"
#include "usb_hid_keys.h"

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

static       int32_t        altitude = 0;
static       fcu_mode       mode     = 0;
static       osThreadId_t   autopilot_task_handle;
static const osThreadAttr_t autopilot_task_attributes =
{
  .name = "autopilot",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static void autopilot_task(void *argument);
static void handle_encoder(void);
static void handle_key(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, const fcu_key key);
static void handle_red_switch(void);

void disable_autopilot(void)
{
  mode &= ~(1UL << MODE_AP);
}

void init_autopilot(void)
{
  autopilot_task_handle = osThreadNew(autopilot_task, NULL, &autopilot_task_attributes);
}

static void autopilot_task(void *argument)
{
  while (1)
  {
    handle_encoder();
    handle_red_switch();
    handle_key(SL1_GPIO_Port, SL1_Pin, FCU_KEY_AP);
    handle_key(SL2_GPIO_Port, SL2_Pin, FCU_KEY_HDG);
    handle_key(SL3_GPIO_Port, SL3_Pin, FCU_KEY_UP);
    handle_key(SL4_GPIO_Port, SL4_Pin, FCU_KEY_NAV);
    handle_key(SL5_GPIO_Port, SL5_Pin, FCU_KEY_ALT);
    handle_key(SL6_GPIO_Port, SL6_Pin, FCU_KEY_DOWN);
    osDelay(1);
  }
}

static void handle_encoder(void)
{
  static GPIO_PinState prev_rot_1_dt = GPIO_PIN_RESET;
  static GPIO_PinState prev_rot_2_dt = GPIO_PIN_RESET;
  static uint32_t disp_state          = SEG_OFF;
  static int32_t  heading_queue       = 0;
  static bool     heading_lock        = false;
  static int32_t  heading_step_locked = 0;

  GPIO_PinState   rot_1_dt            = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
  GPIO_PinState   rot_2_dt            = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);
  uint8_t         hid_report[8]       = { 0 };

  if (STATE_READY != get_state())
  {
    prev_rot_1_dt = HAL_GPIO_ReadPin(ROT_1_DT_GPIO_Port, ROT_1_DT_Pin);
    prev_rot_2_dt = HAL_GPIO_ReadPin(ROT_2_DT_GPIO_Port, ROT_2_DT_Pin);
  }

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
  static fcu_key  key_lock        = 0;
  static uint32_t disp_state      = SEG_OFF;
  static uint32_t prev_disp_state = SEG_CLR;

  uint8_t  hid_report[8]   = { 0 };
  uint32_t sync_mode_state = (get_switch_state() & (1U << SW_SYNC_MODE));

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

  if ((disp_state != prev_disp_state) && (STATE_READY == get_state()))
  {
    set_display(disp_state, altitude);
    prev_disp_state = disp_state;
  }
}

static void handle_red_switch(void)
{
  static GPIO_PinState red_switch;

  uint8_t hid_report[8] = { 0 };

  if (STATE_READY != get_state())
  {
    red_switch = HAL_GPIO_ReadPin(RED_SWITCH_GPIO_Port, RED_SWITCH_Pin);
  }

  if (red_switch != HAL_GPIO_ReadPin(RED_SWITCH_GPIO_Port, RED_SWITCH_Pin))
  {
    hid_report[0] = KEY_MOD_LALT;
    hid_report[2] = KEY_M;

    add_report_to_fifo_queue(hid_report);

    red_switch = HAL_GPIO_ReadPin(RED_SWITCH_GPIO_Port, RED_SWITCH_Pin);
  }
}
