#ifndef _USB_EXTRA_H_
#define _USB_EXTRA_H_

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

#define EXTRA_INTERFACE         3
#define EXTRA_ENDPOINT          4
#define EXTRA_SIZE              8

// from PJRC
int8_t usb_extra_consumer_send(uint16_t bits);
int8_t usb_extra_system_send(uint16_t bits);


// extrakey IN request callback handler
void extra_in_cb(USBDriver* usbp, usbep_t ep);

#endif /* _USB_EXTRA_H_ */