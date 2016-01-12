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

#if defined(F072)
#define BUTTON_GPIO GPIOA
#define BUTTON_PIN  GPIOA_BUTTON
#define BUTTON_MODE PAL_MODE_INPUT
#define BUTTON_ACTIVE PAL_HIGH
#define LED_GPIO    GPIOC
#define LED_PIN     GPIOC_LED_BLUE
#define LED2_GPIO   GPIOC
#define LED2_PIN    GPIOC_LED_ORANGE
#endif

#if defined(F042)
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN  GPIOB_BUTTON
#define BUTTON_MODE PAL_MODE_INPUT
#define BUTTON_ACTIVE PAL_HIGH
#define LED_GPIO    GPIOA
#define LED_PIN     GPIOA_LED_AMBER
#endif

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

  wkup_old_state = 0;

  while(1) {
    wkup_cur_state = palReadPad(BUTTON_GPIO, BUTTON_PIN);
    if(wkup_cur_state != wkup_old_state) {
      chSysLock();
      if(usbGetDriverStateI(&USB_DRIVER) == USB_ACTIVE) {
        chSysUnlock();
        /* just some test code for various reports
         * choose one and comment the others
         */

        /* keyboard test, sends 'n' in nkro mode, 'm' in normal mode */
#ifdef NKRO_ENABLE
        if(wkup_cur_state == BUTTON_ACTIVE) {
          report.nkro.bits[2] |= 2; // 'n'
        } else {
          report.nkro.bits[2] &= 0b11111101;
        }
#else
        report.keys[0] = ((wkup_cur_state == BUTTON_ACTIVE) ? 0x10 : 0); // 'm'
#endif
        send_keyboard(&report);

        /* mouse test, moves the mouse pointer diagonally right and down */
        // send_mouse(&mouse_report);

        /* consumer keys test, sends 'mute audio' */
        // if(wkup_cur_state == BUTTON_ACTIVE) {
        //   send_consumer(AUDIO_MUTE);
        // } else {
        //   send_consumer(0);
        // }

        /* system keys test, sends 'sleep key'
         * on macs it takes a second or two for the system to react
         * I suppose it's to prevent from accidental hits of the sleep key
         */
        // if(wkup_cur_state == BUTTON_ACTIVE) {
        //   send_system(SYSTEM_SLEEP);
        // } else {
        //   send_system(0);
        // }

        /* debug console test, sends the button state and the alphabet
         *  - also blink, to see that the code above is not blocking
         */
        // sendchar(((wkup_cur_state == BUTTON_ACTIVE) ? '1' : '0'));
        // uint8_t n;
        // for(n = 0; n < 26; n++) {
        //   sendchar('A' + n);
        //   sendchar('a' + n);
        // }
        // sendchar('\n');
        // palSetPad(LED2_GPIO, LED2_PIN);
        // chThdSleepMilliseconds(50);
        // palClearPad(LED2_GPIO, LED2_PIN);
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
    palClearPad(LED_GPIO, LED_PIN);
    chThdSleepMilliseconds(time);
    palSetPad(LED_GPIO, LED_PIN);
    chThdSleepMilliseconds(time);
  }
}


/* Main thread
 */
int main(void) {
  /* ChibiOS/RT init */
  halInit();
  chSysInit();

  palSetPad(LED_GPIO, LED_PIN);
  chThdSleepMilliseconds(400);
  palClearPad(LED_GPIO, LED_PIN);

  /* Init USB */
  init_usb_driver(&USB_DRIVER);

  /* Start blinking */
  chThdCreateStatic(waBlinkerThread, sizeof(waBlinkerThread), NORMALPRIO, blinkerThread, NULL);

  /* Wait until the USB is active */
  while(USB_DRIVER.state != USB_ACTIVE)
    chThdSleepMilliseconds(1000);

  chThdSleepMilliseconds(500);

  /* Start the button thread */
  palSetPadMode(BUTTON_GPIO, BUTTON_PIN, BUTTON_MODE);
  chThdCreateStatic(waButtonThread, sizeof(waButtonThread), NORMALPRIO, buttonThread, NULL);

  /* Main loop */
  while(true) {
    chThdSleepMilliseconds(200);
    /* caps lock led status */
#if defined(LED2_GPIO)
    palWritePad(LED2_GPIO, LED2_PIN, ((keyboard_leds() & 2) == 2));
#endif
  }
}
