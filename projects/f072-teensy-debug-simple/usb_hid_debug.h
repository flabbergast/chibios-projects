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

#ifndef _USB_HID_DEBUG_H_
#define _USB_HID_DEBUG_H_

#include "ch.h"
#include "hal.h"

/* The emission queue */
extern output_queue_t usb_output_queue;

/* Initialize the USB Input/Output queues */
void init_usb_queues(void);

/* Initialize the USB driver and bus */
void init_usb_driver(void);

/* Putchar over the USB debug */
msg_t usb_debug_putchar(uint8_t c);

/* Flush output (send everything immediately) */
void usb_debug_flush_output(void);

/* Helper functions from PJRC */
#define pchar(c) usb_debug_putchar(c)
void print(const char *s);
void phex(uint8_t c);
void phex16(uint16_t i);
#endif /* _USB_HID_H_ */
