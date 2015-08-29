#ifndef _USB_CONSOLE_H_
#define _USB_CONSOLE_H_

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

#define CONSOLE_INTERFACE      2
#define CONSOLE_ENDPOINT       3
#define CONSOLE_SIZE           32

// Number of IN reports that can be stored inside the output queue
#define CONSOLE_QUEUE_CAPACITY 2
#define CONSOLE_QUEUE_BUFFER_SIZE (CONSOLE_QUEUE_CAPACITY * CONSOLE_SIZE)

// The emission queue
extern output_queue_t console_queue;

// from PJRC
extern volatile uint8_t console_flush_timer;

// Initialize the USB Input/Output queues
void init_console_queue(void);

// Putchar over the USB debug
msg_t sendchar(uint8_t c);

// Flush output (send everything immediately)
void console_flush_output(void);



// debug IN request callback handler
void console_in_cb(USBDriver* usbp, usbep_t ep);

#endif /* _USB_CONSOLE_H_ */