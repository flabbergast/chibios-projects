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

#include "ch.h"
#include "hal.h"

#include "usb_hid.h"

static struct usb_hid_in_report_s usb_hid_in_report;
static struct usb_hid_out_report_s usb_hid_out_report;

/*
 * Button thread
 *
 * This thread regularely checks the value of the wkup
 * pushbutton. When its state changes, the thread sends a IN report.
 */
static THD_WORKING_AREA(waButtonThread, 128);

static uint8_t wkup_old_state, wkup_cur_state;

static THD_FUNCTION(buttonThread, arg) {
  (void)arg;

  wkup_old_state = 0;

  while(1) {
    wkup_cur_state = palReadPad(GPIOA, GPIOA_BUTTON);
    if(wkup_cur_state != wkup_old_state) {
      chSysLock();
      if(usbGetDriverStateI(&USBD1) == USB_ACTIVE) {
        chSysUnlock();
        /* Build the IN report to be sent */
        usb_build_in_report(&usb_hid_in_report);
        /* Send the IN report */
        usb_send_hid_report(&usb_hid_in_report);
      }else
        chSysUnlock();

      wkup_old_state = wkup_cur_state;
    }
    chThdSleepMilliseconds(50);
  }
}


/*
 * Blue LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waBlinkerThread, 128);
static THD_FUNCTION(blinkerThread, arg) {
  (void)arg;
  chRegSetThreadName("blinkerThread");

  while(true) {
    systime_t time = USBD1.state == USB_ACTIVE ? 250 : 500;
    palClearPad(GPIOC, GPIOC_LED_BLUE);
    chThdSleepMilliseconds(time);
    palSetPad(GPIOC, GPIOC_LED_BLUE);
    chThdSleepMilliseconds(time);
  }
}


/*
 * Main thread
 */
int main(void) {
  /* ChibiOS/RT init */
  halInit();
  chSysInit();

  palSetPad(GPIOC, GPIOC_LED_BLUE);
  chThdSleepMilliseconds(400);
  palClearPad(GPIOC, GPIOC_LED_BLUE);

  /* Init USB */
  init_usb_queues();
  init_usb_driver();

  chThdCreateStatic(waBlinkerThread, sizeof(waBlinkerThread), NORMALPRIO, blinkerThread, NULL);

  /* Wait until the USB is active */
  while(USBD1.state != USB_ACTIVE)
    chThdSleepMilliseconds(1000);

  chThdSleepMilliseconds(500);

  /* Start the button thread */
  chThdCreateStatic(waButtonThread, sizeof(waButtonThread), NORMALPRIO, buttonThread, NULL);

  /*
   * Main loop: waits until a OUT report is received and updates the LED
   */
  while(1) {
    if(chIQReadTimeout
         (&usb_input_queue, (uint8_t *)&usb_hid_out_report,
         USB_HID_OUT_REPORT_SIZE, TIME_INFINITE) == USB_HID_OUT_REPORT_SIZE) {
      if(usb_hid_out_report.led_value == 0)
        palSetPad(GPIOC, GPIOC_LED_RED);
      else
        palClearPad(GPIOC, GPIOC_LED_RED);
    }
  }
}
