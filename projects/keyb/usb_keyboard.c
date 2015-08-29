#include "ch.h"
#include "hal.h"

#include "usb_keyboard.h"

uint8_t keyboard_idle = 0;
uint8_t keyboard_protocol = 1;
uint8_t keyboard_led_stats = 0;

report_keyboard_t keyboard_report_sent = { // this depends in KEYBOARD_REPORT_KEYS... / NKRO
  .mods = 0,
  .reserved = 0,
  {0, 0, 0, 0, 0, 0}
  };

volatile uint16_t keyboard_idle_count = 0;

// keyboard IN callback hander (a kbd report has made it IN)
void kbd_in_cb(USBDriver* usbp, usbep_t ep) {
  // STUB
  (void)usbp;
  (void)ep;
}

// start-of-frame handler
// i guess it would be better to re-implement using timers,
//  so that this is not going to have to be checked every 1ms
void kbd_sof_cb(USBDriver *usbp) {
#ifdef NKRO_ENABLE
  if(!keyboard_nkro && keyboard_idle) {
#else
  if(keyboard_idle) {
#endif
    keyboard_idle_count++;
    palWritePad(GPIOC, GPIOC_LED_RED, keyboard_led_stats & 0b10);
    if(keyboard_idle_count == 4*(uint16_t)keyboard_idle) {
      keyboard_idle_count = 0;
      // TODO: are we sure we want the KBD_ENDPOINT?
      usbPrepareTransmit(usbp, KBD_ENDPOINT, (uint8_t *)&keyboard_report_sent, KBD_SIZE);
      osalSysLockFromISR();
      usbStartTransmitI(usbp, KBD_ENDPOINT);
      osalSysUnlockFromISR();
    }
  }
}

#ifdef NKRO_ENABLE
// keyboard2 IN callback hander (a kbd2 report has made it IN)
void kbd2_in_cb(USBDriver* usbp, usbep_t ep) {
  // STUB
  (void)usbp;
  (void)ep;
}
#endif

uint8_t keyboard_leds(void) {
  return keyboard_led_stats;
}

// prepare and start sending a report IN
// not callable from ISR or locked state
void send_keyboard(report_keyboard_t *report) {
  osalSysLock();
  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) {
    osalSysUnlock();
    return;
  }
  osalSysUnlock();

#ifdef NKRO_ENABLE
  if(keyboard_nkro) {  // NKRO protocol
    usbPrepareTransmit(&USB_DRIVER, KBD2_ENDPOINT, (uint8_t *)report, KBD2_SIZE);
    osalSysLock();
    usbStartTransmitI(&USB_DRIVER, KBD2_ENDPOINT);
    osalSysUnlock();
  } else
#endif
  { // boot protocol
    usbPrepareTransmit(&USB_DRIVER, KBD_ENDPOINT, (uint8_t *)report, KBD_SIZE);
    osalSysLock();
    usbStartTransmitI(&USB_DRIVER, KBD_ENDPOINT);
    osalSysUnlock();
  }
  keyboard_report_sent = *report;
}

