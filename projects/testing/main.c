/*
 * (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 * Based on ChibiOS 3.0.1 demo code, license below.
 * Licensed under the Apache License, Version 2.0.
 */

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "usbcfg.h"
#include "serial.h"
#include "output.h"

#include "eep24lc.h"

/*===========================================================================*/
/* Target-specific defs.                                                     */
/*===========================================================================*/

#if defined(TEENSY30) || defined(TEENSY32) || defined(TEENSYLC)
/* Teensy 3.0 and 3.2 */
#define BUTTON_GPIO TEENSY_PIN2_IOPORT
#define BUTTON_PIN TEENSY_PIN2
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO TEENSY_PIN13_IOPORT
#define LED_PIN TEENSY_PIN13
#define LED_ON() palSetPad(LED_GPIO, LED_PIN)
#define LED_OFF() palClearPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
#endif

#if defined(WF)
/* Whitefox */
#define BUTTON_GPIO GPIOD
#define BUTTON_PIN 0
#define BUTTON_ACTIVE PAL_HIGH
#define BUTTON_MODE PAL_MODE_INPUT_PULLDOWN
#define LED_GPIO GPIOA
#define LED_PIN 5
#define LED_ON() palSetPad(LED_GPIO, LED_PIN)
#define LED_OFF() palClearPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
#endif

#if defined(MCHCK)
/* MCHCK */
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN 17
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO GPIOB
#define LED_PIN 16
#define LED_ON() palSetPad(LED_GPIO, LED_PIN)
#define LED_OFF() palClearPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
#endif

#if defined(KL27Z)
/* KL27Z breakout */
#define BUTTON_GPIO GPIO_BUTTON
#define BUTTON_PIN PIN_BUTTON
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO GPIO_LED
#define LED_PIN PIN_LED
#define LED_ON() palSetPad(LED_GPIO, LED_PIN)
#define LED_OFF() palClearPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
#endif

#if defined(KL25Z)
/* Freedom KL25Z board */
// no button so just use PTB0 */
#define BUTTON_GPIO GPIOC
#define BUTTON_PIN 2
#define BUTTON_ACTIVE PAL_LOW
#define BUTTON_MODE PAL_MODE_INPUT_PULLUP
#define LED_GPIO GPIO_LED_BLUE
#define LED_PIN PIN_LED_BLUE
#define LED_ON() palClearPad(LED_GPIO, LED_PIN)
#define LED_OFF() palSetPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
#endif

#if defined(F042)
/* STM32F042 board */
#define BUTTON_GPIO GPIOB
#define BUTTON_PIN GPIOB_BUTTON
#define BUTTON_ACTIVE PAL_HIGH
#define BUTTON_MODE PAL_MODE_INPUT
#define LED_GPIO GPIOA
#define LED_PIN GPIOA_LED_AMBER
#define LED_ON() palSetPad(LED_GPIO, LED_PIN)
#define LED_OFF() palClearPad(LED_GPIO, LED_PIN)
#define LED_TOGGLE() palTogglePad(LED_GPIO, LED_PIN)
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
    // LED_TOGGLE();
    // chThdSleepMilliseconds(time);

    // blink whenever led_blink is set
    if(led_blink != 0) {
      LED_ON();
      chThdSleepMilliseconds(20);
      LED_OFF();
      led_blink = 0;
    }
    chThdSleepMilliseconds(10);
  }
}

static inline void delayMicroseconds(uint32_t) __attribute__((always_inline, unused));
static inline void delayMicroseconds(uint32_t usec)
{
// #if F_CPU == 96000000
        uint32_t n = usec << 5;
// #elif F_CPU == 72000000
        // uint32_t n = usec << 5; // XXX Not accurate, assembly snippet needs to be updated
// #elif F_CPU == 48000000
        // uint32_t n = usec << 4;
// #elif F_CPU == 24000000
        // uint32_t n = usec << 3;
// #endif
        asm volatile(
                ".syntax unified"                       "\n\t"
                "L_%=_delayMicroseconds:"               "\n\t"
                "subs   %0, #1"                         "\n\t"
                "bne    L_%=_delayMicroseconds"         "\n"
                : "+r" (n) :
        );
}

/*===========================================================================
 * Main.
 *===========================================================================*/

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

#if defined(KL25Z)
  /* Turn off RGB LED */
  palSetPad(GPIO_LED_RED, PIN_LED_RED); /* red */
  palSetPad(GPIO_LED_GREEN, PIN_LED_GREEN); /* green */
  palSetPad(GPIO_LED_BLUE, PIN_LED_BLUE); /* blue */
  /* disable the I2C pins routed onboard to the 3-axis sensor */
  palSetPadMode(GPIOE, 25, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, 24, PAL_MODE_OUTPUT_PUSHPULL);
#endif /* KL25Z */

  /*
   * Setup button pad
   */
  palSetPadMode(BUTTON_GPIO, BUTTON_PIN, BUTTON_MODE);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waBlinkThr, sizeof(waBlinkThr), NORMALPRIO, BlinkThr, NULL);

  /*
   * Init I2C
   */
  eep24lc_i2c_init();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&OUTPUT_CHANNEL);
  sduStart(&OUTPUT_CHANNEL, &serusbcfg);

  /*
   * Initialise serial driver.
   */
  serial_init();

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  uint8_t t = 0;
  msg_t msg;

  uint8_t txbuf[10] = { 0, 0, 3,4,5,6,7,8,9,10 };
  uint8_t rxbuf[10] = {0};

#if defined(PROJECT_USE_SERIAL)
#define OUT SERIAL_DRIVER
#else
#define OUT OUTPUT_CHANNEL
#endif

  /*
   * Normal main() thread activity.
   */
  while(true) {
    if((palReadPad(BUTTON_GPIO, BUTTON_PIN) == BUTTON_ACTIVE)) {
    // if((palReadPad(BUTTON_GPIO, BUTTON_PIN) == BUTTON_ACTIVE) && (OUTPUT_CHANNEL.config->usbp->state == USB_ACTIVE)) {
      /* sdWrite(&OUTPUT_CHANNEL, (uint8_t *)"hello world\r\n", 13); */
      // chprintf((BaseSequentialStream *)&OUTPUT_CHANNEL, "Hello world\r\n");
      // chnPutTimeout(&OUTPUT_CHANNEL, 'W', TIME_IMMEDIATE);          
      led_blink = 1;
      chnPutTimeout(&OUT, 't', TIME_IMMEDIATE);
      msg = i2cMasterTransmit(&I2C_DRIVER, EEP24LC_ADDR, txbuf, 2, rxbuf, 10);
      // msg = i2cMasterTransmit(&I2C_DRIVER, EEP24LC_ADDR, txbuf, 2, NULL, 0);
      chprintf((BaseSequentialStream *)&OUT, "%d\r\n",msg);
      // chThdSleepMilliseconds(1);
      // msg = i2cMasterReceiveTimeout(&I2C_DRIVER, EEP24LC_ADDR, rxbuf, 10, TIME_INFINITE);
      // chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "%d\r\n",msg);
      for(t=0; t<10; t++) {
        phex(&OUT,rxbuf[t]);
      }
      pent(&OUT);
      i2cMasterTransmit(&I2C_DRIVER, EEP24LC_ADDR, txbuf, 10, NULL, 0);
      // chnPutTimeout(&OUTPUT_CHANNEL, 't', TIME_IMMEDIATE);
      // msg = eep24lc_write_byte(1,8);
      // chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "%d\r\n",msg);
      // chprintf((BaseSequentialStream *)&OUTPUT_CHANNEL, "%d\r\n",msg);
      chThdSleepMilliseconds(1000);
      // eep24lc_read_byte(1,&t);
      // eep24lc_cur_addr_read_byte(&t);
      // phex(&SERIAL_DRIVER,t);
      // pent(&SERIAL_DRIVER);
      // phex(&OUTPUT_CHANNEL,t);
      // pent(&OUTPUT_CHANNEL);
      // eep24lc_write_byte(1,++t);
      // phex(&OUTPUT_CHANNEL,(*(uint8_t *)(&fok))); // so it's LSB
      // phex(&SERIAL_DRIVER,(*(uint8_t *)(&fok))); // so it's LSB
      // chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "Hello world\r\n");
      // i2cMasterTransmitTimeout(&I2CD1, ISSI_ADDR_DEFAULT, tx, 1, rx, 1, MS2ST(5));
      // status = i2cMasterReceiveTimeout(&I2CD1, ISSI_ADDR_DEFAULT, rx, 0, MS2ST(5));
      // shutdown
      // status = issi_write_register(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x00);
      // status = issi_select_page(0xB);
      // __asm("BKPT #0\n") ;

      // if(status == (msg_t)-2) {
      //   led_blink = 1;
      // }
      /* chnWrite((BaseChannel *)&OUTPUT_CHANNEL, (uint8_t *)"Hello, world\r\n", 14); */
    }

    // Serial echo
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
