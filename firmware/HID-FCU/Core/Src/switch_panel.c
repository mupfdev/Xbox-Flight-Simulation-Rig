/*
 * switch_panel.c
 *
 *  Created on: Aug 16, 2023
 *      Author: mail
 */

#include <stdbool.h>
#include <stdint.h>
#include <switch_panel.h>
#include "cmsis_os.h"
#include "display.h"
#include "state_handler.h"
#include "stm32f1xx_hal.h"
#include "usb_hid_keys.h"
#include "usb_hid_handler.h"

#define OFF_POSITION 1

typedef enum
{
  SW_BATTERY_ON,
  SW_BATTERY_OFF,
  SW_GENERATOR_ON,
  SW_GENERATOR_OFF,
  SW_GEN_RESET_ON,
  SW_GEN_RESET_OFF,
  SW_AUX_F_PUMP_ON,
  SW_AUX_F_PUMP_OFF,
  SW_AVIONIC_BUS1_ON,
  SW_AVIONIC_BUS1_OFF,
  SW_AVIONIC_BUS2_ON,
  SW_AVIONIC_BUS2_OFF,
  SW_IGNITION_ON,
  SW_IGNITION_OFF,
  SW_STARTER_ON,
  SW_STARTER_OFF,
  SW_NAV_LT_ON,
  SW_NAV_LT_OFF,
  SW_BEACON_ON,
  SW_BEACON_OFF,
  SW_LANDING_LIGHT_ON,
  SW_LANDING_LIGHT_OFF,
  SW_LANDING_LIGHT_POS_UP,
  SW_LANDING_LIGHT_POS_DOWN,
  SW_INSTR_LIGHT_ON,
  SW_INSTR_LIGHT_OFF,
  SW_CABIN_LIGHT_ON,
  SW_CABIN_LIGHT_OFF,
  SW_ANTI_ICE_ON,
  SW_ANTI_ICE_OFF,
  SW_PROP_DEICE_ON,
  SW_PROP_DEICE_OFF,
  SW_PARKING_BRAKE_ON,
  SW_PARKING_BRAKE_OFF,
  SW_FUEL_VALVE_ON,
  SW_FUEL_VALVE_OFF,
  SW_TAIL_WHEEL_LOCK_ON,
  SW_TAIL_WHEEL_LOCK_OFF,
  SW_RESERVED_4_ON,
  SW_RESERVED_4_OFF,
  SW_RESERVED_5_ON,
  SW_RESERVED_5_OFF,
  SW_RESERVED_6_ON,
  SW_RESERVED_6_OFF,
  SW_RESERVED_7_ON,
  SW_RESERVED_7_OFF,
  SW_RESERVED_8_ON,
  SW_RESERVED_8_OFF,
  SW_RESERVED_9_ON,
  SW_RESERVED_9_OFF,
  SW_RESERVED_10_ON,
  SW_RESERVED_10_OFF,
  SW_RESERVED_11_ON,
  SW_RESERVED_11_OFF,
  SW_RESERVED_12_ON,
  SW_RESERVED_12_OFF,
  SW_RESERVED_13_ON,
  SW_RESERVED_13_OFF,
  SW_RESERVED_14_ON,
  SW_RESERVED_14_OFF,
  SW_RESERVED_15_ON,
  SW_RESERVED_15_OFF,
  SW_SYNC_MODE_ON,
  SW_SYNC_MODE_OFF

} switch_event;

extern I2C_HandleTypeDef hi2c1;

static uint32_t switch_state      = 0x00;
static uint32_t prev_switch_state = 0x00;

static       osThreadId_t   switch_panel_task_handle;
static const osThreadAttr_t switch_panel_task_attributes =
{
  .name = "switch_panel",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static int  poll_switch_panel(void);
static void send_switch_event(switch_event event);
static void switch_panel_task(void *argument);

void init_switch_panel(void)
{
  switch_panel_task_handle = osThreadNew(switch_panel_task, NULL, &switch_panel_task_attributes);
  poll_switch_panel();
}

uint32_t get_switch_state(void)
{
  return switch_state;
}

static int poll_switch_panel(void)
{
  HAL_StatusTypeDef status;
  uint8_t           buffer[4] = { 0xff, 0xff, 0xff, 0xff };

  prev_switch_state = switch_state;

  /* Primary switch panel. */
  status = HAL_I2C_Master_Receive(&hi2c1, 0x45, &buffer[0], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }
  else
  {
    switch_state = (switch_state & 0xffffff00) | buffer[0];
  }

  status = HAL_I2C_Master_Receive(&hi2c1, 0x41, &buffer[1], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }
  else
  {
    switch_state = (switch_state & 0xffff00ff) | ((uint32_t)buffer[1]<<  8);
  }

  /* Secondary switch panel. */
  status = HAL_I2C_Master_Receive(&hi2c1, 0x47, &buffer[2], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }
  else
  {
    switch_state = (switch_state & 0xff00ffff) | ((uint32_t)buffer[2] << 16);
  }

  status = HAL_I2C_Master_Receive(&hi2c1, 0x43, &buffer[3], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }
  else
  {
    switch_state = (switch_state & 0x00ffffff) | ((uint32_t)buffer[3] << 24);
  }

  return 0;
}

static void send_switch_event(switch_event event)
{
  uint8_t hid_report[8] = { 0 };

  switch (event)
  {
    case SW_BATTERY_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_1;
      break;
    case SW_BATTERY_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_2;

      set_state(STATE_OFF);
      clear_display();

      break;
    case SW_GENERATOR_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_3;
      break;
    case SW_GENERATOR_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_4;
      break;
    case SW_GEN_RESET_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_5;
      break;
    case SW_GEN_RESET_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_6;
      break;
    case SW_AUX_F_PUMP_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_7;
      break;
    case SW_AUX_F_PUMP_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_8;
      break;
    case SW_AVIONIC_BUS1_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_9;
      break;
    case SW_AVIONIC_BUS1_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_0;
      break;
    case SW_AVIONIC_BUS2_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_Q;
      break;
    case SW_AVIONIC_BUS2_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_W;

      set_state(STATE_OFF);
      clear_display();

      break;
    case SW_IGNITION_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_E;
      break;
    case SW_IGNITION_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_R;
      break;
    case SW_STARTER_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_T;
      break;
    case SW_STARTER_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_Y;
      break;
    case SW_NAV_LT_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_U;
      break;
    case SW_NAV_LT_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_I;
      break;
    case SW_BEACON_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_O;
      break;
    case SW_BEACON_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_P;
      break;
    case SW_LANDING_LIGHT_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_A;
      break;
    case SW_LANDING_LIGHT_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_S;
      break;
    case SW_LANDING_LIGHT_POS_UP:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_D;
      break;
    case SW_LANDING_LIGHT_POS_DOWN:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_F;
      break;
    case SW_INSTR_LIGHT_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_G;
      break;
    case SW_INSTR_LIGHT_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_H;
      break;
    case SW_CABIN_LIGHT_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_J;
      break;
    case SW_CABIN_LIGHT_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_K;
      break;
    case SW_ANTI_ICE_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_L;
      break;
    case SW_ANTI_ICE_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_Z;
      break;
    case SW_PROP_DEICE_ON:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_X;
      break;
    case SW_PROP_DEICE_OFF:
      hid_report[0] = KEY_MOD_LALT;
      hid_report[2] = KEY_C;
      break;
    case SW_PARKING_BRAKE_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_1;
      break;
    case SW_PARKING_BRAKE_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_2;
      break;
    case SW_FUEL_VALVE_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_3;
      break;
    case SW_FUEL_VALVE_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_4;
      break;
    case SW_TAIL_WHEEL_LOCK_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_5;
      break;
    case SW_TAIL_WHEEL_LOCK_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_6;
      break;
    case SW_RESERVED_4_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_7;
      break;
    case SW_RESERVED_4_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_8;
      break;
    case SW_RESERVED_5_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_9;
      break;
    case SW_RESERVED_5_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_0;
      break;
    case SW_RESERVED_6_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_Q;
      break;
    case SW_RESERVED_6_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_W;
      break;
    case SW_RESERVED_7_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_E;
      break;
    case SW_RESERVED_7_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_R;
      break;
    case SW_RESERVED_8_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_T;
      break;
    case SW_RESERVED_8_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_Y;
      break;
    case SW_RESERVED_9_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_U;
      break;
    case SW_RESERVED_9_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_I;
      break;
    case SW_RESERVED_10_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_O;
      break;
    case SW_RESERVED_10_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_P;
      break;
    case SW_RESERVED_11_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_A;
      break;
    case SW_RESERVED_11_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_S;
      break;
    case SW_RESERVED_12_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_D;
      break;
    case SW_RESERVED_12_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_F;
      break;
    case SW_RESERVED_13_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_G;
      break;
    case SW_RESERVED_13_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_H;
      break;
    case SW_RESERVED_14_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_J;
      break;
    case SW_RESERVED_14_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_K;
      break;
    case SW_RESERVED_15_ON:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_L;
      break;
    case SW_RESERVED_15_OFF:
      hid_report[0] = KEY_MOD_RALT;
      hid_report[2] = KEY_Z;
      break;
    case SW_SYNC_MODE_ON:
    case SW_SYNC_MODE_OFF:
      return;
  }

  add_report_to_fifo_queue(hid_report);
}

static void switch_panel_task(void *argument)
{
  while (1)
  {
    poll_switch_panel();

    for (switch_id id = 0; id < SW_SYNC_MODE; id = id + 1)
    {
      static bool ign_triggered_once = false;

      uint32_t bit_state      = (switch_state & (1U << id));
      uint32_t prev_bit_state = (prev_switch_state & (1U << id));

      if (bit_state != prev_bit_state)
      {
        switch (id)
        {
          case SW_BATTERY:
            if (bit_state > 0) /* OFF */
            {
              ign_triggered_once = false;
              send_switch_event(SW_BATTERY_OFF);
            }
            else
            {
              send_switch_event(SW_BATTERY_ON);
            }
            break;
          case SW_GENERATOR:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_GENERATOR_OFF);
            }
            else
            {
              send_switch_event(SW_GENERATOR_ON);
            }
            break;
          case SW_GEN_RESET:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_GEN_RESET_OFF);
            }
            else
            {
              send_switch_event(SW_GEN_RESET_ON);
            }
            break;
          case SW_AUX_F_PUMP:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_AUX_F_PUMP_OFF);
            }
            else
            {
              send_switch_event(SW_AUX_F_PUMP_ON);
            }
            break;
          case SW_AVIONIC_BUS1:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_AVIONIC_BUS1_OFF);
            }
            else
            {
              send_switch_event(SW_AVIONIC_BUS1_ON);
            }
            break;
          case SW_AVIONIC_BUS2:
            if (bit_state > 0) /* OFF */
            {
              ign_triggered_once = false;
              send_switch_event(SW_AVIONIC_BUS2_OFF);
            }
            else
            {
              send_switch_event(SW_AVIONIC_BUS2_ON);
            }
            break;
          case SW_IGNITION:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_IGNITION_OFF);
            }
            else
            {
              /* Hack: For some reason this is necessary so that the ingition
               * switch is reliably toggled the first time. */
              if (false == ign_triggered_once)
              {
                send_switch_event(SW_IGNITION_ON);
                send_switch_event(SW_IGNITION_ON);
                HAL_Delay(50);
                ign_triggered_once = true;
              }

              send_switch_event(SW_IGNITION_ON);
            }
            break;
          case SW_STARTER:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_STARTER_OFF);
            }
            else
            {
              send_switch_event(SW_STARTER_ON);
            }
            break;
          case SW_NAV_LT:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_NAV_LT_OFF);
            }
            else
            {
              send_switch_event(SW_NAV_LT_ON);
            }
            break;
          case SW_BEACON:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_BEACON_OFF);
            }
            else
            {
              send_switch_event(SW_BEACON_ON);
            }
            break;
          case SW_LANDING_LIGHT:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_LANDING_LIGHT_OFF);
            }
            else
            {
              send_switch_event(SW_LANDING_LIGHT_ON);
            }
            break;
          case SW_LANDING_LIGHT_POS:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_LANDING_LIGHT_POS_DOWN);
            }
            else
            {
              send_switch_event(SW_LANDING_LIGHT_POS_UP);
            }
            break;
          case SW_INSTR_LIGHT:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_INSTR_LIGHT_OFF);
            }
            else
            {
              send_switch_event(SW_INSTR_LIGHT_ON);
            }
            break;
          case SW_CABIN_LIGHT:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_CABIN_LIGHT_OFF);
            }
            else
            {
              send_switch_event(SW_CABIN_LIGHT_ON);
            }
            break;
          case SW_ANTI_ICE:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_ANTI_ICE_OFF);
            }
            else
            {
              send_switch_event(SW_ANTI_ICE_ON);
            }
            break;
          case SW_PROP_DEICE:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_PROP_DEICE_OFF);
            }
            else
            {
              send_switch_event(SW_PROP_DEICE_ON);
            }
            break;
          case SW_PARKING_BRAKE:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_PARKING_BRAKE_OFF);
            }
            else
            {
              send_switch_event(SW_PARKING_BRAKE_ON);
            }
            break;
          case SW_FUEL_VALVE:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_FUEL_VALVE_OFF);
            }
            else
            {
              send_switch_event(SW_FUEL_VALVE_ON);
            }
            break;
          case SW_TAIL_WHEEL_LOCK:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_TAIL_WHEEL_LOCK_OFF);
            }
            else
            {
              send_switch_event(SW_TAIL_WHEEL_LOCK_ON);
            }
            break;
          case SW_RESERVED_4:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_4_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_4_ON);
            }
            break;
          case SW_RESERVED_5:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_5_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_5_ON);
            }
            break;
          case SW_RESERVED_6:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_6_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_6_ON);
            }
            break;
          case SW_RESERVED_7:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_7_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_7_ON);
            }
            break;
          case SW_PAUSE:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_8_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_8_ON);
            }
            break;
          case SW_RESERVED_9:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_9_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_9_ON);
            }
            break;
          case SW_RESERVED_10:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_10_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_10_ON);
            }
            break;
          case SW_RESERVED_11:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_11_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_11_ON);
            }
            break;
          case SW_RESERVED_12:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_12_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_12_ON);
            }
            break;
          case SW_RESERVED_13:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_13_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_13_ON);
            }
            break;
          case SW_RESERVED_14:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_14_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_14_ON);
            }
            break;
          case SW_RESERVED_15:
            if (bit_state > 0) /* OFF */
            {
              send_switch_event(SW_RESERVED_15_OFF);
            }
            else
            {
              send_switch_event(SW_RESERVED_15_ON);
            }
            break;
          case SW_SYNC_MODE:
          default:
            break;
        }
      }
    }
    osDelay(1);
  }
}
