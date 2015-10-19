/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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

static THD_WORKING_AREA(waBlinkThread, 128);
static THD_FUNCTION(BlinkThread, arg) {
  (void)arg;

  while(TRUE) {
    palSetPad(GPIO_LED_BLUE, PIN_LED_BLUE);
    chThdSleepMilliseconds(700);
    // palClearPad(GPIO_LED_BLUE, PIN_LED_BLUE);
    chThdSleepMilliseconds(700);
  }    
}

/* Breathing Sleep LED brighness(PWM On period) table
 * (64[steps] * 4[duration]) / 64[PWM periods/s] = 4 second breath cycle
 *
 * http://www.wolframalpha.com/input/?i=%28sin%28+x%2F64*pi%29**8+*+255%2C+x%3D0+to+63
 * (0..63).each {|x| p ((sin(x/64.0*PI)**8)*255).to_i }
 */
static const uint8_t breathing_table[64] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 6, 10,
15, 23, 32, 44, 58, 74, 93, 113, 135, 157, 179, 199, 218, 233, 245, 252,
255, 252, 245, 233, 218, 199, 179, 157, 135, 113, 93, 74, 58, 44, 32, 23,
15, 10, 6, 4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Low Power Timer ISR */
OSAL_IRQ_HANDLER(KINETIS_LPTMR0_IRQ_VECTOR) {
  OSAL_IRQ_PROLOGUE();

  /* Software PWM
   * timer:1111 1111 1111 1111
   *       \_____/\/ \_______/____  count(0-255)
   *          \    \______________  duration of step(4)
   *           \__________________  index of step table(0-63)
   */

  // this works for cca 65536 irqs/sec
  static union {
    uint16_t row;
    struct {
      uint8_t count:8;
      uint8_t duration:2;
      uint8_t index:6;
    } pwm;
  } timer = { .row = 0 };

  timer.row++;
  
  // LED on
  if (timer.pwm.count == 0) {
    palClearPad(GPIO_LED_RED, PIN_LED_RED);
  }
  // LED off
  if (timer.pwm.count == breathing_table[timer.pwm.index]) {
    palSetPad(GPIO_LED_RED, PIN_LED_RED);
  }

  /* Reset the counter */
  LPTMR0->CSR |= LPTMRx_CSR_TCF;

  OSAL_IRQ_EPILOGUE();
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

  /* Turn off the leds */
  palSetPad(GPIO_LED_RED, PIN_LED_RED); /* red */
  palSetPad(GPIO_LED_GREEN, PIN_LED_GREEN); /* green */
  palSetPad(GPIO_LED_BLUE, PIN_LED_BLUE); /* blue */

  /*
   * Create the blink thread.
   */
  chThdCreateStatic(waBlinkThread, sizeof(waBlinkThread), NORMALPRIO, BlinkThread, NULL);

  /* Clock sources for LPTMR defines */
  #define LPTMR_CLOCK_MCGIRCLK 0 /* 4MHz clock */
  #define LPTMR_CLOCK_LPO      1 /* 1kHz clock */
  #define LPTMR_CLOCK_ERCLK32K 2 /* external 32kHz crystal */
  #define LPTMR_CLOCK_OSCERCLK 3 /* output from OSC */

  /* Work around inconsistencies in Freescale naming */
  #if !defined(SIM_SCGC5_LPTMR)
  #define SIM_SCGC5_LPTMR SIM_SCGC5_LPTIMER
  #endif

  /* Make sure the clock to the LPTMR is enabled */
  SIM->SCGC5 |= SIM_SCGC5_LPTMR;
  /* Reset LPTMR settings */
  LPTMR0->CSR = 0;
  /* Set the compare value */
  LPTMR0->CMR = 0;  // trigger every time

  /* Set up clock source and prescaler: 3 OPTIONS */
  /* Software PWM
   *  ______           ______           __
   * |  ON  |___OFF___|  ON  |___OFF___|   ....
   * |<-------------->|<-------------->|<- ....
   *     PWM period       PWM period
   *
   * R                interrupts/period[resolution]
   * F                periods/second[frequency]
   * R * F            interrupts/second
   */

  /* === OPTION 1 === */
  #if 0
  //  1kHz LPO
  //  No prescaler => 1024 irqs/sec
  LPTMR0->PSR = LPTMRx_PSR_PCS(LPTMR_CLOCK_LPO)|LPTMRx_PSR_PBYP;
  #endif /* OPTION 1 */

  /* === OPTION 2 === */
  #if 1
  //  IRC (usually 4MHz; 2 or 8 MHz for KL27Z)
  MCG->C2 |= MCG_C2_IRCS; // fast (4MHz) internal ref clock
  #if defined(KL27)  // divide the 8MHz IRC by 2, to have the same MCGIRCLK speed as others 
  MCG->MC |= MCG_MC_LIRC_DIV2_DIV2;
  #endif /* KL27 */
  MCG->C1 |= MCG_C1_IRCLKEN; // enable internal ref clock
  //  to work in stop mode, also MCG_C1_IREFSTEN
  //  Divide 4MHz by 2^N (N=6) => 62500 irqs/sec =>
  //  => approx F=61, R=256, duration = 4
  LPTMR0->PSR = LPTMRx_PSR_PCS(LPTMR_CLOCK_MCGIRCLK)|LPTMRx_PSR_PRESCALE(6);
  #endif /* OPTION 2 */

  /* === OPTION 3 === */
  #if 0
  //  OSC output (external crystal), usually 8MHz or 16MHz
  OSC0->CR |= OSC_CR_ERCLKEN; // enable ext ref clock
  //  to work in stop mode, also OSC_CR_EREFSTEN
  //  Divide by 2^N (N=7 => 62500 irqs/sec)
  LPTMR0->PSR = LPTMRx_PSR_PCS(LPTMR_CLOCK_OSCERCLK)|LPTMRx_PSR_PRESCALE(8);
  #endif /* OPTION 3 */
  /* === END OPTIONS === */

  /* Interrupt on TCF set (compare flag) */
  nvicEnableVector(LPTMR0_IRQn, 2); // vector, priority
  LPTMR0->CSR |= LPTMRx_CSR_TIE;

  /* Start the timer */
  LPTMR0->CSR |= LPTMRx_CSR_TEN;
  /* Wait for counter to reach compare value */
  // while (!(LPTMR0_CSR & LPTMR_CSR_TCF_MASK));
  /* Disable counter and Clear Timer Compare Flag */
  // LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while(TRUE) {
    chThdSleepMilliseconds(500);
  }

  return 0;
}
