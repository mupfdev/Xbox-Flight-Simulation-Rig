/*
 * levers.c
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdint.h>
#include <stdlib.h>
#include "ads1115.h"
#include "cmsis_os.h"
#include "levers.h"
#include "switch_panel.h"
#include "usb_hid_keys.h"
#include "usb_hid_handler.h"

#define PROP_LOWER_LIMIT       600
#define PROP_CENTER          14914
#define PROP_UPPER_LIMIT     26300

#define THROTTLE_LOWER_LIMIT    10
#define THROTTLE_CENTER      11435
#define THROTTLE_UPPER_LIMIT 25300

#define MIXTURE_LOWER_LIMIT     10
#define MIXTURE_CENTER       13740
#define MIXTURE_UPPER_LIMIT  26300

typedef enum
{
  AXIS_PROP = 0,
  AXIS_THROTTLE,
  AXIS_MIXTURE

} axis_index_t;

static void handle_levers(void);
static void levers_task(void *argument);

static       osThreadId_t   levers_task_handle;
static const osThreadAttr_t levers_task_attributes =
{
  .name = "levers",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

void init_levers(void)
{
  ads1115_init();
  levers_task_handle = osThreadNew(levers_task, NULL, &levers_task_attributes);
}

static void handle_levers(void)
{
  static uint16_t axis_data[3] = { 0 };

  uint8_t hid_report[8] = { 0 };

  for (axis_index_t index = 0; index <= AXIS_MIXTURE; index += 1)
  {
    static uint8_t hit_counter = 0;
    static int64_t prev_percentage[3] = { 50, 50, 50 };

    uint8_t  deviation;
    uint8_t  channel;
    int16_t  lower_limit;
    int16_t  center;
    int16_t  upper_limit;
    uint32_t sync_mode_state = (get_switch_state() & (1U << SW_SYNC_MODE));
    int64_t  percentage[3];

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

    deviation = abs((int)(prev_percentage[index] - percentage[index]));
    if (((deviation > 0) || (0 == percentage[index])) && (index == AXIS_THROTTLE))
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
        /* Tbd. */

        break;
      case AXIS_THROTTLE:
        hid_report[0] = KEY_MOD_LCTRL;

        switch (percentage[index])
        {
          case 40:
            hit_counter   = 0;
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
            if ((0 == percentage[index]) && (hit_counter <= 50))
            {
              hit_counter += 1;
              hid_report[2] = KEY_F11;
            }
            else if ((prev_percentage[index] > percentage[index]) && (percentage[index] < 40))
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
        hid_report[0] = KEY_MOD_RCTRL;



        break;
    }

    if (sync_mode_state != 0)
    {
      add_report_to_fifo_queue(hid_report);
    }

    prev_percentage[index] = percentage[index];
  }
}

static void levers_task(void *argument)
{
  while (1)
  {
    handle_levers();
    osDelay(20);
  }
}
