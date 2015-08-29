#ifndef _USB_DEBUG_H_
#define _USB_DEBUG_H_

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

#define DEBUG_INTERFACE         2
#define DEBUG_TX_ENDPOINT       3
#define DEBUG_TX_SIZE           32

// Number of IN reports that can be stored inside the output queue
#define USB_OUTPUT_QUEUE_CAPACITY 2
#define USB_OUTPUT_QUEUE_BUFFER_SIZE (USB_OUTPUT_QUEUE_CAPACITY * DEBUG_TX_SIZE)

// The emission queue
extern output_queue_t usb_output_queue;

// from PJRC
extern volatile uint8_t debug_flush_timer;

// Initialize the USB Input/Output queues
void init_usb_debug_queue(void);

// Putchar over the USB debug
msg_t usb_debug_putchar(uint8_t c);

// Flush output (send everything immediately)
void usb_debug_flush_output(void);



// debug IN request callback handler
void debug_in_cb(USBDriver* usbp, usbep_t ep);

#endif /* _USB_DEBUG_H_ */