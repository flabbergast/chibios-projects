#ifndef _USB_MOUSE_H_
#define _USB_MOUSE_H_

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

#define MOUSE_INTERFACE         1
#define MOUSE_ENDPOINT          2
#define MOUSE_SIZE              8

// from PJRC
extern uint8_t usb_mouse_protocol;

// mouse IN request callback handler
void mouse_in_cb(USBDriver* usbp, usbep_t ep);

int8_t usb_mouse_send(int8_t x, int8_t y, int8_t wheel_v, int8_t wheel_h, uint8_t buttons);
void usb_mouse_print(int8_t x, int8_t y, int8_t wheel_v, int8_t wheel_h, uint8_t buttons);



#endif /* _USB_MOUSE_H_ */