/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

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
#include "ch_test.h"

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palClearLine(LINE_LED_RED);
    chThdSleepMilliseconds(500);
    palSetLine(LINE_LED_RED);
    chThdSleepMilliseconds(500);
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
   * Route USART3 to PC4/TX/pin1 PC5/RX/pin0
   */
  palSetLineMode(LINE_PIN0, PAL_MODE_ALTERNATE(7));
  palSetLineMode(LINE_PIN1, PAL_MODE_ALTERNATE(7));

  /*
   * Activates the serial driver 2 using the driver default configuration.
   */
  sdStart(&SD3, NULL);

  // palSetLineMode(LINE_LED_GREEN, PAL_MODE_OUTPUT_PUSHPULL);

  palClearLine(LINE_LED_GREEN);
  chThdSleepMilliseconds(500);
  palSetLine(LINE_LED_GREEN);


  // palSetPadMode(GPIOC, 10, PAL_MODE_OUTPUT_PUSHPULL);
  // palTogglePad(GPIOC, 10);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    if (palReadLine(LINE_BUTTON)) {
      palToggleLine(LINE_LED_BLUE);
      // test_execute((BaseSequentialStream *)&SD3);
    }
    chThdSleepMilliseconds(500);
  }
}
