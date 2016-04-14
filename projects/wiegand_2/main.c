/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Based on ChibiOS 3.0.1 demo code, license below.
 * Licensed under the Apache License, Version 2.0.
 */

/*
 *
 *  ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "shell.h"
#include "chprintf.h"

#include "version.h"

#include "usbcfg.h"
#include "wiegand.h"

uint8_t wieg_test_buf[26] = {0, 1,1,0,0,1,1,0,0,1,1,0,0, 1,1,0,0,1,1,0,0,1,1,0,0, 1};

/*===========================================================================*/
/* Target-specific defs.                                                     */
/*===========================================================================*/

#if defined(TEENSY30) || defined(TEENSY32)
/* Teensy 3.0 and 3.2*/
#define BUTTON_GPIO TEENSY_PIN2_IOPORT
#define BUTTON_PIN TEENSY_PIN2
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO TEENSY_PIN13_IOPORT
#define LED_PIN TEENSY_PIN13
#endif

#if defined(MCHCK)
/* MCHCK */
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN 17
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO GPIOB
#define LED_PIN 16
#endif

#if defined(KL27Z)
/* KL27Z breakout */
#define BUTTON_GPIO GPIO_BUTTON
#define BUTTON_PIN PIN_BUTTON
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO GPIO_LED
#define LED_PIN PIN_LED
#endif

#if defined(F042)
/* STM32F042 board */
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN GPIOB_BUTTON
#define BUTTON_ACTIVE PAL_HIGH
#define BUTTON_MODE PAL_MODE_INPUT
#define LED_GPIO GPIOA
#define LED_PIN GPIOA_LED_AMBER
#endif

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

static uint8_t unlock_pw_recved = 0;

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(1024)

static void cmd_mode(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  if((argc == 0) || (argc > 1)) {
    chprintf(chp, "Usage: mode [26|34|err|debug]\r\n");
    chprintf(chp, "Current mode: ");
    if( print_mode & MODE_DEBUG ) {
      chprintf(chp, "debug\r\n");
      return;
    } else if( print_mode & MODE_ERR ) {
      chprintf(chp, "err\r\n");
      return;
    } else if( print_mode & MODE_26 ) {
      chprintf(chp, "26\r\n");
      return;
    } else if( print_mode & MODE_34 ) {
      chprintf(chp, "34\r\n");
      return;
    } else {
      chprintf(chp, "unknown?\r\n");
      return;      
    }
  } else {
    if( !strncmp(argv[0], "debug", 5) ) {
      print_mode = MODE_DEBUG;
      chprintf(chp, "New mode: debug\r\n");
      return;
    } else if( !strncmp(argv[0], "err", 3) ) {
      print_mode = MODE_ERR;
      chprintf(chp, "New mode: err\r\n");
      return;
    } else if( !strncmp(argv[0], "26", 2) ) {
      print_mode = MODE_26;
      chprintf(chp, "New mode: 26\r\n");
      return;
    } else if( !strncmp(argv[0], "34", 2) ) {
      print_mode = MODE_34;
      chprintf(chp, "New mode: 34\r\n");
      return;
    } else {
      chprintf(chp, "Unknown mode, mode NOT changed\r\n");
      return;
    }
  }

}

static void cmd_savemode(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;

  if((argc == 0) || (argc > 1)) {
    chprintf(chp, "Usage: savemode yes\r\n");
    return;
  }

  if(!strncmp(argv[0],"yes",3)) {
    write_print_mode(print_mode);
    chprintf(chp, "Mode saved\r\n");    
  } else {
    chprintf(chp, "Mode NOT saved\r\n");    
  }
}

static void cmd_version(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  chprintf(chp, FW_REV_FULLSTRING);
}

static const ShellCommand commands[] = {
  {"mode", cmd_mode},
  {"savemode", cmd_savemode},
  {"version", cmd_version},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&OUTPUT_CHANNEL,
  commands
};


/*===========================================================================
 * Generic code.
 *===========================================================================*/

/*
 * LED blinker thread, times are in milliseconds.
 */
volatile uint8_t led_blink = 0;
static THD_WORKING_AREA(waBlinkThr, 128);
static THD_FUNCTION(BlinkThr, arg) {
  (void)arg;
  chRegSetThreadName("blinker");

  /* LED setup */
  palSetPadMode(LED_GPIO, LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);

  while(true) {
    // just periodically blink
    // systime_t time = serusbcfg.usbp->state == USB_ACTIVE ? 250 : 500;
    // palTogglePad(LED_GPIO, LED_PIN);
    // chThdSleepMilliseconds(time);

    // blink whenever led_blink is set
    if(led_blink != 0) {
      palSetPad(LED_GPIO, LED_PIN);
      chThdSleepMilliseconds(20);
      palClearPad(LED_GPIO, LED_PIN);
      led_blink = 0;
    }
    chThdSleepMilliseconds(10);
  }
}


/*===========================================================================
 * Main loop.
 *===========================================================================*/

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

#if defined(F042)
  /* This is needed to remap the USB pins PA11,PA12 onto the default PA9,PA10
   * so that the USB works. After halInit (which changes that register).
   * This also means that USART1 can't be used, as it is on PA9,PA10.
   */
  SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
#endif /* F042 */

  chSysInit();

  /*
   * Setup button pad
   */
  palSetPadMode(BUTTON_GPIO, BUTTON_PIN, BUTTON_MODE);

  /*
   * Create the blinker thread.
   */
  chThdCreateStatic(waBlinkThr, sizeof(waBlinkThr), NORMALPRIO, BlinkThr, NULL);

  /*
   * Setup things for wiegand
   */
  wieg_init();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&OUTPUT_CHANNEL);
  sduStart(&OUTPUT_CHANNEL, &serusbcfg);

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  led_blink = 1;

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * The main loop.
   */
  /*
  while(true) {
    if((palReadPad(BUTTON_GPIO, BUTTON_PIN) == BUTTON_ACTIVE) && (OUTPUT_CHANNEL.config->usbp->state == USB_ACTIVE)) {
      // sdWrite(&OUTPUT_CHANNEL, (uint8_t *)"hello world\r\n", 13);
      // chprintf((BaseSequentialStream *)&OUTPUT_CHANNEL, "Hello world\r\n");
      chnPutTimeout(&OUTPUT_CHANNEL, 'W', TIME_IMMEDIATE);
      // wieg_send(wieg_test_buf, 26);
      led_blink = 1;
      chThdSleepMilliseconds(200);
      // chnWrite((BaseChannel *)&OUTPUT_CHANNEL, (uint8_t *)"Hello, world\r\n", 14);
    }

    msg_t charbuf;
    charbuf = chnGetTimeout(&OUTPUT_CHANNEL, TIME_IMMEDIATE);
    if(charbuf != Q_TIMEOUT) {
      switch(charbuf) {
        default:
          chprintf((BaseSequentialStream *)&OUTPUT_CHANNEL, "Haloooo\r\n");
          break;
      }
      // chnPutTimeout(&OUTPUT_CHANNEL, (uint8_t)charbuf, TIME_IMMEDIATE);
      // if(charbuf == '\r') {
        // chnPutTimeout(&OUTPUT_CHANNEL, '\n', TIME_IMMEDIATE);          
      // }
    }

    chThdSleepMilliseconds(50);
  }
  */

  /*
   * Normal main() thread activity, spawning shells.
   */
  while (true) {
    msg_t c;
    if(serusbcfg.usbp->state == USB_ACTIVE) {
      c = chnGetTimeout(&OUTPUT_CHANNEL, TIME_IMMEDIATE);
      if(c != Q_TIMEOUT) {
        switch(c) {
          case 'k':
            if(unlock_pw_recved==0) {
              unlock_pw_recved++;
              led_blink = 1;
            }
            break;
          case 'o':
            if(unlock_pw_recved==1) {
              unlock_pw_recved++;
              led_blink = 1;
            }
            break;
          case 'j':
            if(unlock_pw_recved==2) {
              unlock_pw_recved++;
              led_blink = 1;
            }
            break;
          case 'a':
            if(unlock_pw_recved==3) {
              unlock_pw_recved++;
              led_blink = 1;
            }
            break;
        }
      }
      if(unlock_pw_recved == 4) {
        unlock_pw_recved = 0;      
        thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                                "shell", NORMALPRIO + 1,
                                                shellThread, (void *)&shell_cfg1);
        chThdWait(shelltp);               /* Waiting termination.             */
      }
    }
    chThdSleepMilliseconds(50);
  }
}
