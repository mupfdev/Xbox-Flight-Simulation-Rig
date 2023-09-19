/*
 * usb_hid_handler.c
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "usb_hid_handler.h"
#include "usb_hid_keys.h"
#include "usbd_hid.h"

#define FIFO_QUEUE_SIZE 100

static uint8_t fifo_queue_index = 0;
static uint8_t fifo_queue[FIFO_QUEUE_SIZE][8];

static       SemaphoreHandle_t queue_mutex;
static       osThreadId_t      usb_hid_handler_task_handle;
static const osThreadAttr_t    usb_hid_handler_task_attributes =
{
  .name = "usb_hid_handler",
  .stack_size = 128 * 2,
  .priority = (osPriority_t) osPriorityHigh,
};

static void handle_fifo_queue(void);
static void send_report(uint8_t hid_report[]);
static void usb_hid_handler_task(void *argument);

void add_report_to_fifo_queue(uint8_t hid_report[])
{
  if (fifo_queue_index > FIFO_QUEUE_SIZE)
  {
    fifo_queue_index = 0;
  }

  for (int i = 0; i < 8; i += 1)
  {
    fifo_queue[fifo_queue_index][i] = hid_report[i];
  }

  fifo_queue_index += 1;
}

void init_usb_hid_handler(void)
{
  memset(fifo_queue, 0, sizeof(fifo_queue));

  queue_mutex = xSemaphoreCreateMutex();

  usb_hid_handler_task_handle = osThreadNew(usb_hid_handler_task, NULL, &usb_hid_handler_task_attributes);
}

static void handle_fifo_queue(void)
{
  uint8_t hid_report[8];
  int     first_valid_item;
  bool    valid_item_found = false;

  for (first_valid_item = 0; first_valid_item < FIFO_QUEUE_SIZE; first_valid_item += 1)
  {
    for (int i = 0; i < 8; i += 1)
    {
      hid_report[i] = fifo_queue[first_valid_item][i];
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

#if 0
    while (pdFALSE == xSemaphoreTake(queue_mutex, 10 / portTICK_PERIOD_MS))
    {
      osDelay(1);
    }
#endif

    /* Clear last valid item. */
    for (int i = 0; i < 8; i += 1)
    {
      fifo_queue[first_valid_item][i] = 0x00;
    }

#if 0
    xSemaphoreGive(queue_mutex);
#endif
  }
}

static void send_report(uint8_t hid_report[])
{
  extern USBD_HandleTypeDef hUsbDeviceFS;

  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
  osDelay(40);

  /* Release keys. */
  for (int i = 0; i < 8; i += 1)
  {
    hid_report[i] = KEY_NONE;
  }
  USBD_HID_SendReport(&hUsbDeviceFS, hid_report, 8);
  osDelay(10);
}

static void usb_hid_handler_task(void *argument)
{
  while (1)
  {
    handle_fifo_queue();
    osDelay(1);
  }
}
