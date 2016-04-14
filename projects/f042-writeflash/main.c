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

#include "chprintf.h"

#include "flash.h"

#include "usbcfg.h"

/*===========================================================================
 * Output definitions.
 *===========================================================================*/
#define phex4(chn, c) chnPutTimeout(chn, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE)
#define phex(chn, c) phex4(chn, (c>>4)); phex4(chn, (c&15))
#define phex16(chn, c) phex(chn, (uint8_t)(c>>8)); phex(chn, (uint8_t)c)
#define phex24(chn, c) phex16(chn, (uint32_t)((c>>8)&0xFFFF)); phex(chn, (uint8_t)c)
#define phex32(chn, c) phex16(chn, (c>>16)); phex16(chn, c&0xFFFF)
#define pent(chn) chnWriteTimeout(chn, (const uint8_t *)"\r\n", 2, TIME_IMMEDIATE);

#if defined(F042)
/* STM32F042 board */
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN GPIOB_BUTTON
#define BUTTON_ACTIVE PAL_HIGH
#define BUTTON_MODE PAL_MODE_INPUT
#define LED_GPIO GPIOA
#define LED_PIN GPIOA_LED_AMBER

/* Address - beginning of the last 1k page (on 32kB MCUs) */
#define FLASH_ADDR 0x08007C00
#endif

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

uint16_t w = 0;

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
   * Creates the blinker thread.
   */
  chThdCreateStatic(waBlinkThr, sizeof(waBlinkThr), NORMALPRIO, BlinkThr, NULL);

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

  /*
   * The main loop.
   */
  while(true) {
    if((palReadPad(BUTTON_GPIO, BUTTON_PIN) == BUTTON_ACTIVE) && (OUTPUT_CHANNEL.config->usbp->state == USB_ACTIVE)) {
      /* sdWrite(&OUTPUT_CHANNEL, (uint8_t *)"hello world\r\n", 13); */
      // chprintf((BaseSequentialStream *)&OUTPUT_CHANNEL, "Hello world\r\n");
      chnPutTimeout(&OUTPUT_CHANNEL, 'W', TIME_IMMEDIATE);
      led_blink = 1;
      phex4(&OUTPUT_CHANNEL,w);
      pent(&OUTPUT_CHANNEL);
      w++;
      osalSysLock();
      flash_unlock();
      flash_erasepage(FLASH_ADDR);
      flash_write16(FLASH_ADDR,w);
      flash_lock();
      osalSysUnlock();
      w = flash_read16(FLASH_ADDR);
      chThdSleepMilliseconds(2000);
      /* chnWrite((BaseChannel *)&OUTPUT_CHANNEL, (uint8_t *)"Hello, world\r\n", 14); */
    }

    msg_t charbuf;
    charbuf = chnGetTimeout(&OUTPUT_CHANNEL, TIME_IMMEDIATE);
    if(charbuf != Q_TIMEOUT) {
      chnPutTimeout(&OUTPUT_CHANNEL, (uint8_t)charbuf, TIME_IMMEDIATE);
      if(charbuf == '\r') {
        chnPutTimeout(&OUTPUT_CHANNEL, '\n', TIME_IMMEDIATE);          
      }
    }

    chThdSleepMilliseconds(50);
  }
}
