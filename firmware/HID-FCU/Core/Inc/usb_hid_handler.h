/*
 * usb_hid_handler.h
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef USB_HID_HANDLER_H
#define USB_HID_HANDLER_H

#include <stdint.h>

void add_report_to_fifo_queue(uint8_t hid_report[]);
void init_usb_hid_handler(void);

#endif /* USB_HID_HANDLER_H */
