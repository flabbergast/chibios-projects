/*
 * (c) 2015 flabberast <s3+flabbergast@sdfeu.org>
 *
 * Based on the following work:
 *  - Guillaume Duc's raw hid example (MIT License)
 *    https://github.com/guiduc/usb-hid-chibios-example
 *  - PJRC Teensy examples (MIT License)
 *    https://www.pjrc.com/teensy/usb_keyboard.html
 *  - hasu's TMK keyboard code (GPL v2 and some code Modified BSD)
 *    https://github.com/tmk/tmk_keyboard/
 *  - ChibiOS demo code (Apache 2.0 License)
 *    http://www.chibios.org
 *
 * Since some GPL'd code is used, this work is licensed under
 * GPL v2 or later.
 */

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

report_keyboard_t report = {{0}};

#ifdef MOUSE_ENABLE
report_mouse_t mouse_report = {
  .buttons = 0,
  .x = 50,
  .y = 50,
  .v = 0,
  .h = 0
};
#endif

/* Button thread
 *
 * This thread regularely checks the value of the wkup
 * pushbutton. When its state changes, something happens.
 */
static THD_WORKING_AREA(waButtonThread, 128);
static uint8_t wkup_old_state, wkup_cur_state;
static THD_FUNCTION(buttonThread, arg) {
  (void)arg;

  wkup_old_state = 1;

  while(1) {
    wkup_cur_state = palReadPad(GPIO_BUTTON, PIN_BUTTON);
    if(wkup_cur_state != wkup_old_state) {
      chSysLock();
      if(usbGetDriverStateI(&USB_DRIVER) == USB_ACTIVE) {
        chSysUnlock();
        /* just some test code for various reports
         * choose one and comment the others
         */

        /* keyboard test, sends 'n' in nkro mode, 'm' in normal mode */
#ifdef NKRO_ENABLE
        if(!wkup_cur_state) {
          // report.nkro.bits[2] |= 2; // 'n'
          report.nkro.bits[7] |= 2; // caps lock
        } else {
          // report.nkro.bits[2] &= 0b11111101;
          report.nkro.bits[7] &= 0b11111101;
        }
#else
        report.keys[0] = (!wkup_cur_state ? 0x10 : 0); // 'm'
#endif
        send_keyboard(&report);

        /* mouse test, moves the mouse pointer diagonally right and down */
        // send_mouse(&mouse_report);

        /* consumer keys test, sends 'mute audio' */
        // if(wkup_cur_state) {
        //   send_consumer(AUDIO_MUTE);
        // } else {
        //   send_consumer(0);
        // }

        /* system keys test, sends 'sleep key'
         * on macs it takes a second or two for the system to react
         * I suppose it's to prevent from accidental hits of the sleep key
         */
        // if(wkup_cur_state) {
        //   send_system(SYSTEM_SLEEP);
        // } else {
        //   send_system(0);
        // }

        /* debug console test, sends the button state and the alphabet
         *  - also blink, to see that the code above is not blocking
         */
        // sendchar((wkup_cur_state ? '1' : '0'));
        // uint8_t n;
        // for(n = 0; n < 26; n++) {
        //   sendchar('A' + n);
        //   sendchar('a' + n);
        // }
        // sendchar('\n');
        // palSetPad(GPIOC, GPIOC_LED_ORANGE);
        // chThdSleepMilliseconds(50);
        // palClearPad(GPIOC, GPIOC_LED_ORANGE);
      } else
        chSysUnlock();

      wkup_old_state = wkup_cur_state;
    }
    chThdSleepMilliseconds(50);
  }
}


/* Blue LED blinker thread, times are in milliseconds.
 *
 * Blinks fast when USB is active, slower when it isn't.
 */
static THD_WORKING_AREA(waBlinkerThread, 128);
static THD_FUNCTION(blinkerThread, arg) {
  (void)arg;
  chRegSetThreadName("blinkerThread");

  while(true) {
    systime_t time = USB_DRIVER.state == USB_ACTIVE ? 250 : 500;
    palClearPad(GPIO_LED_BLUE, PIN_LED_BLUE);
    chThdSleepMilliseconds(time);
    palSetPad(GPIO_LED_BLUE, PIN_LED_BLUE);
    chThdSleepMilliseconds(time);
  }
}

#if defined(DEBUG_USB)
SerialConfig s0cfg = {
  115200
};
#endif /* DEBUG_USB */

/* Main thread
 */
int main(void) {
  /* ChibiOS/RT init */
  halInit();
  chSysInit();

  palSetPad(GPIO_LED_RED, PIN_LED_RED);
  palSetPad(GPIO_LED_GREEN, PIN_LED_GREEN);
  chThdSleepMilliseconds(400);
  palSetPad(GPIO_LED_BLUE, PIN_LED_BLUE);

#if defined(DEBUG_USB)
  sdStart(&SD1, &s0cfg);
#endif /* DEBUG_USB */

#if defined(DEBUG_USB)
  usb_debug_init();
#endif /* DEBUG_USB */

  /* Init USB */
  init_usb_driver(&USB_DRIVER);

  /* Start blinking */
  chThdCreateStatic(waBlinkerThread, sizeof(waBlinkerThread), NORMALPRIO, blinkerThread, NULL);

  /* Wait until the USB is active */
  while(USB_DRIVER.state != USB_ACTIVE)
    chThdSleepMilliseconds(1000);

  chThdSleepMilliseconds(500);

  /* Start the button thread */
  chThdCreateStatic(waButtonThread, sizeof(waButtonThread), NORMALPRIO, buttonThread, NULL);

  /* Main loop */
  while(true) {
    chThdSleepMilliseconds(200);
    /* caps lock led status */
    palWritePad(GPIO_LED_RED, PIN_LED_RED, ((keyboard_leds() & 2) != 2));
  }
}
