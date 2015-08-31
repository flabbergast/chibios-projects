/*
 * (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 * Based on ChibiOS 3.0.1 demo code, license below.
 * Licensed under the Apache License, Version 2.0.
 */

/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

static int ledState = 1;

/*
 * Amber LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker1");
  /*
   *while (true) {
   *  palClearPad(GPIOA, GPIOA_LED_AMBER);
   *  chThdSleepMilliseconds(250);
   *  palSetPad(GPIOA, GPIOA_LED_AMBER);
   *  chThdSleepMilliseconds(250);
   *}
   */
  while(true) {
    palWritePad(GPIOA, GPIOA_LED_AMBER, ledState);
    chThdSleepMilliseconds(100);
  }
}

/*
 * Check button thread
 */
static THD_WORKING_AREA(waThread2, 128);
static THD_FUNCTION(Thread2, arg) {
  (void)arg;
  chRegSetThreadName("checkbutton1");

  uint8_t newstate,state = PAL_LOW;

  while(true) {
    if( palReadPad(GPIOB, GPIOB_BUTTON) != state ) {
      chThdSleepMilliseconds(20); // debounce
      newstate = palReadPad(GPIOB, GPIOB_BUTTON);
      if( newstate != state ) {
        state = newstate;
        if (newstate == PAL_HIGH) {
          ledState = !ledState;
        }
      }
    }
    chThdSleepMilliseconds(20);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Create the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Create the button check thread.
   */
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state, when the button is
   * pressed ... nothing happens.
   */
  while (true) {
    chThdSleepMilliseconds(500);
  }
}
