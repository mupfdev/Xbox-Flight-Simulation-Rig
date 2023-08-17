/*
 * switch-panel.c
 *
 *  Created on: Aug 16, 2023
 *      Author: mail
 */

#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "switch-panel.h"
#include "usb_device.h"
#include "usbd_hid.h"
#include "usb_hid_keys.h"

#define OFF_POSITION 1

typedef enum
{
  SW_BATTERY_ON,
  SW_BATTERY_OFF,
  SW_GENERATOR_ON,
  SW_GENERATOR_OFF,
  SW_GEN_RESET_ON,
  SW_GEN_RESET_OFF,
  SW_IGNITION_ON,
  SW_IGNITION_OFF,
  SW_AUX_F_PUMP_ON,
  SW_AUX_F_PUMP_OFF,
  SW_AVIONIC_BUS1_ON,
  SW_AVIONIC_BUS1_OFF,
  SW_AVIONIC_BUS2_ON,
  SW_AVIONIC_BUS2_OFF,
  SW_STARTER_ON,
  SW_STARTER_OFF,
  SW_NAV_LT_ON,
  SW_NAV_LT_OFF,
  SW_BEACON_ON,
  SW_BEACON_OFF,
  SW_PROP_DEICE_ON,
  SW_PROP_DEICE_OFF,
  SW_LANDING_LIGHT_ON,
  SW_LANDING_LIGHT_OFF,
  SW_LANDING_LIGHT_POS_UP,
  SW_LANDING_LIGHT_POS_DOWN,
  SW_INSTR_LIGHT_ON,
  SW_INSTR_LIGHT_OFF,
  SW_CABIN_LIGHT_ON,
  SW_CABIN_LIGHT_OFF,
  SW_ANTI_ICE_ON,
  SW_ANTI_ICE_OFF

} switch_event;

extern I2C_HandleTypeDef hi2c1;

uint16_t switch_state = 0x00;

static uint16_t prev_switch_state = 0x00;

static int poll_switch_panel(void);
static void send_switch_event(switch_event event);

void handle_switch_panel(void)
{
  poll_switch_panel();

  for (int n = 0; n < 16; n = n + 1)
  {
    uint16_t bit_state      = (switch_state & (1U << n));
    uint16_t prev_bit_state = (prev_switch_state & (1U << n));

    if (bit_state != prev_bit_state)
    {
      switch (n)
      {
        case 0:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_BATTERY_OFF);
          }
          else
          {
            send_switch_event(SW_BATTERY_ON);
          }
          break;
        case 1:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_GENERATOR_OFF);
          }
          else
          {
            send_switch_event(SW_GENERATOR_ON);
          }
          break;
        case 2:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_GEN_RESET_OFF);
          }
          else
          {
            send_switch_event(SW_GEN_RESET_ON);
          }
          break;
        case 3:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_IGNITION_OFF);
          }
          else
          {
            static bool ign_triggered_once = false;

            /* Hack: For some reason this is necessary so that the ingition
             * switch is reliably toggled the first time. */
            if (false == ign_triggered_once)
            {
              send_switch_event(SW_IGNITION_ON);
              HAL_Delay(100);
              ign_triggered_once = true;
            }

            send_switch_event(SW_IGNITION_ON);
          }
          break;
        case 4:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_AUX_F_PUMP_OFF);
          }
          else
          {
            send_switch_event(SW_AUX_F_PUMP_ON);
          }
          break;
        case 5:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_AVIONIC_BUS1_OFF);
          }
          else
          {
            send_switch_event(SW_AVIONIC_BUS1_ON);
          }
          break;
        case 6:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_AVIONIC_BUS2_OFF);
          }
          else
          {
            send_switch_event(SW_AVIONIC_BUS2_ON);
          }
          break;
        case 7:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_STARTER_OFF);
          }
          else
          {
            send_switch_event(SW_STARTER_ON);
          }
          break;
        case 8:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_NAV_LT_OFF);
          }
          else
          {
            send_switch_event(SW_NAV_LT_ON);
          }
          break;
        case 9:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_BEACON_OFF);
          }
          else
          {
            send_switch_event(SW_BEACON_ON);
          }
          break;
        case 10:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_PROP_DEICE_OFF);
          }
          else
          {
            send_switch_event(SW_PROP_DEICE_ON);
          }
          break;
        case 11:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_LANDING_LIGHT_OFF);
          }
          else
          {
            send_switch_event(SW_LANDING_LIGHT_ON);
          }
          break;
        case 12:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_LANDING_LIGHT_POS_DOWN);
          }
          else
          {
            send_switch_event(SW_LANDING_LIGHT_POS_UP);
          }
          break;
        case 13:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_INSTR_LIGHT_OFF);
          }
          else
          {
            send_switch_event(SW_INSTR_LIGHT_ON);
          }
          break;
        case 14:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_CABIN_LIGHT_OFF);
          }
          else
          {
            send_switch_event(SW_CABIN_LIGHT_ON);
          }
          break;
        case 15:
          if (bit_state > 0) /* OFF */
          {
            send_switch_event(SW_ANTI_ICE_OFF);
          }
          else
          {
            send_switch_event(SW_ANTI_ICE_ON);
          }
          break;
      }
    }
  }
}

static int poll_switch_panel(void)
{
  HAL_StatusTypeDef status;
  uint8_t           buffer[2] = { 0 };

  prev_switch_state = switch_state;

  status = HAL_I2C_Master_Receive(&hi2c1, 0x40, &buffer[0], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }

  status = HAL_I2C_Master_Receive(&hi2c1, 0x44, &buffer[1], 1, 100);
  if (HAL_OK != status)
  {
    return -1;
  }

  switch_state = (switch_state & 0xff00) |  buffer[0];
  switch_state = (switch_state & 0x00ff) | ((uint16_t)buffer[1] <<  8);

  return 0;
}

static void send_switch_event(switch_event event)
{
  extern USBD_HandleTypeDef hUsbDeviceFS;
  extern uint8_t hid_report[8];

  switch (event)
  {
    case SW_BATTERY_ON:
      hid_report[2] = KEY_Q;
      break;
    case SW_BATTERY_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_Q;
      break;
    case SW_GENERATOR_ON:
      hid_report[2] = KEY_W;
      break;
    case SW_GENERATOR_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_W;
      break;
    case SW_GEN_RESET_ON:
      hid_report[2] = KEY_E;
      break;
    case SW_GEN_RESET_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_E;
      break;
    case SW_AUX_F_PUMP_ON:
      hid_report[2] = KEY_R;
      break;
    case SW_AVIONIC_BUS1_ON:
      hid_report[2] = KEY_T;
      break;
    case SW_AVIONIC_BUS1_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_T;
      break;
    case SW_AVIONIC_BUS2_ON:
      hid_report[2] = KEY_Y;
      break;
    case SW_AVIONIC_BUS2_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_Y;
      break;
    case SW_IGNITION_ON:
    case SW_IGNITION_OFF:
      hid_report[2] = KEY_U;
      break;
    case SW_STARTER_ON:
    case SW_STARTER_OFF:
      hid_report[2] = KEY_I;
      break;
    case SW_NAV_LT_ON:
      hid_report[2] = KEY_O;
      break;
    case SW_NAV_LT_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_O;
      break;
    case SW_BEACON_ON:
      hid_report[2] = KEY_P;
      break;
    case SW_BEACON_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_P;
      break;
    case SW_LANDING_LIGHT_ON:
    case SW_LANDING_LIGHT_OFF:
      hid_report[2] = KEY_A;
      break;
    case SW_LANDING_LIGHT_POS_UP:
      hid_report[2] = KEY_S;
      break;
    case SW_LANDING_LIGHT_POS_DOWN:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_S;
      break;
    case SW_INSTR_LIGHT_ON:
      hid_report[2] = KEY_D;
      break;
    case SW_INSTR_LIGHT_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_D;
      break;
    case SW_CABIN_LIGHT_ON:
      hid_report[2] = KEY_F;
      break;
    case SW_CABIN_LIGHT_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_F;
      break;
    case SW_ANTI_ICE_ON:
      hid_report[2] = KEY_G;
      break;
    case SW_ANTI_ICE_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_G;
      break;
    case SW_PROP_DEICE_ON:
      hid_report[2] = KEY_H;
      break;
    case SW_PROP_DEICE_OFF:
      hid_report[0] = KEY_MOD_LSHIFT;
      hid_report[2] = KEY_H;
      break;
    default:
      return;
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
