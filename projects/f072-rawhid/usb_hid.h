/*
 * Copyright (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 *
 * Based on the work of Guillaume Duc, original licence below.
 * The original license applies to the whole current file.
 */

/*
 *
 * Copyright (c) 2014 Guillaume Duc <guillaume@guiduc.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef _USB_HID_H_
#define _USB_HID_H_

#include "ch.h"
#include "hal.h"

/* Size in bytes of the IN report (board->PC) */
#define USB_HID_IN_REPORT_SIZE 2
/* Size in bytes of the OUT report (PC->board) */
#define USB_HID_OUT_REPORT_SIZE 2

/* Number of OUT reports that can be stored inside the input queue */
#define USB_INPUT_QUEUE_CAPACITY 2
/* Number of IN reports that can be stored inside the output queue */
#define USB_OUTPUT_QUEUE_CAPACITY 2

#define USB_INPUT_QUEUE_BUFFER_SIZE (USB_INPUT_QUEUE_CAPACITY * USB_HID_OUT_REPORT_SIZE)
#define USB_OUTPUT_QUEUE_BUFFER_SIZE (USB_OUTPUT_QUEUE_CAPACITY * USB_HID_IN_REPORT_SIZE)

/*
 * Content of the IN report (board->PC)
 * - 1 byte: sequence number
 * - 1 byte: current value of the WKUP pushbutton
 *
 * => 2 bytes
 */
struct usb_hid_in_report_s {
  uint8_t sequence_number;
  uint8_t wkup_pb_value;
};

/*
 * Content of the OUT report (PC->board)
 * - 1 byte: sequence number
 * - 1 byte: LED value
 *
 * => 2 bytes
 */
struct usb_hid_out_report_s {
  uint8_t sequence_number;
  uint8_t led_value;
};

/* The reception queue */
extern input_queue_t usb_input_queue;
/* The emission queue */
extern output_queue_t usb_output_queue;

/* Initialize the USB Input/Output queues */
void init_usb_queues(void);

/* Initialize the USB driver and bus */
void init_usb_driver(void);

/* Queue a report to be sent */
int usb_send_hid_report(struct usb_hid_in_report_s *report);

/* Prepare an IN report */
void usb_build_in_report(struct usb_hid_in_report_s *report);
#endif /* _USB_HID_H_ */
