#ifndef _USB_KEYBOARD_H_
#define _USB_KEYBOARD_H_

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

/*------------------------------------------------------------------*
 * Keyboard descriptor setting
 *------------------------------------------------------------------*/
#define KBD_INTERFACE   0
#define KBD_ENDPOINT    1
#define KBD_SIZE        8
#define KBD_REPORT_KEYS   (KBD_SIZE - 2)

// secondary keyboard
#ifdef NKRO_ENABLE
#define NKRO_INTERFACE    4
#define NKRO_ENDPOINT     5
#define NKRO_SIZE         16
#define NKRO_REPORT_KEYS  (NKRO_SIZE - 1)
#endif

// this defines report_keyboard_t and computes REPORT_SIZE defines
#include "tmk_common/report.h"

// these are set/read in usb_main, since that't where SETUP reqs are handled
extern uint8_t keyboard_idle;
extern uint8_t keyboard_protocol;
extern bool keyboard_nkro;
extern uint16_t keyboard_led_stats;

extern report_keyboard_t keyboard_report_sent;

// keyboard IN request callback handler
void kbd_in_cb(USBDriver *usbp, usbep_t ep);
// start-of-frame handler
void kbd_sof_cb(USBDriver *usbp);

#ifdef NKRO_ENABLE
// nkro IN callback hander
void nkro_in_cb(USBDriver* usbp, usbep_t ep);
#endif

void send_keyboard(report_keyboard_t *report);
uint8_t keyboard_leds(void);

#endif /* _USB_KEYBOARD_H_ */