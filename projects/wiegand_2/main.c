/*
 * (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
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

/*===========================================================================
 * USB related stuff.
 *===========================================================================*/

/*
 * Endpoints to be used for USBD1.
 */
#define USBD1_DATA_REQUEST_EP           1
#define USBD1_DATA_AVAILABLE_EP         1
#define USBD1_INTERRUPT_REQUEST_EP      2

/*
 * Serial over USB Driver structure.
 */
static SerialUSBDriver SDU1;

/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE(0x0110,               /* bcdUSB (1.1).                    */
                  0x02,                 /* bDeviceClass (CDC).              */
                  0x00,                 /* bDeviceSubClass.                 */
                  0x00,                 /* bDeviceProtocol.                 */
                  0x40,                 /* bMaxPacketSize.                  */
                  0x0179,               /* idVendor.                        */
#if defined(TEENSY30) || defined(TEENSY32)
                  0x0002,               /* idProduct.                       */
#elif defined(MCHCK)
                  0x0003,               /* idProduct.                       */
#elif defined(F042)
                  0x0004,               /* idProduct.                       */
#elif defined(KL27Z)
                  0x0005,               /* idProduct.                       */
#else
                  0x0001,               /* idProduct.                       */
#endif
                  0x0200,               /* bcdDevice.                       */
                  1,                    /* iManufacturer.                   */
                  2,                    /* iProduct.                        */
                  3,                    /* iSerialNumber.                   */
                  1)                    /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
  sizeof vcom_device_descriptor_data,
  vcom_device_descriptor_data
};

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_configuration_descriptor_data[67] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(67,            /* wTotalLength.                    */
                         0x02,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0,             /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         50),           /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE(0x00,              /* bInterfaceNumber.                */
                     0x00,              /* bAlternateSetting.               */
                     0x01,              /* bNumEndpoints.                   */
                     0x02,              /* bInterfaceClass (Communications
                                         * Interface Class, CDC section
                                         * 4.2).                            */
                     0x02,              /* bInterfaceSubClass (Abstract
                                         * Control Model, CDC section 4.3).   */
                     0x01,              /* bInterfaceProtocol (AT commands,
                                        *  CDC section 4.4).                */
                     0),                /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  USB_DESC_BYTE(5),                     /* bLength.                         */
  USB_DESC_BYTE(0x24),                  /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x00),                  /* bDescriptorSubtype (Header
                                         * Functional Descriptor.           */
  USB_DESC_BCD(0x0110),                 /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  USB_DESC_BYTE(5),                     /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24),                  /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x01),                  /* bDescriptorSubtype (Call Management
                                         * Functional Descriptor).          */
  USB_DESC_BYTE(0x00),                  /* bmCapabilities (D0+D1).          */
  USB_DESC_BYTE(0x01),                  /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  USB_DESC_BYTE(4),                     /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24),                  /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x02),                  /* bDescriptorSubtype (Abstract
                                         * Control Management Descriptor).  */
  USB_DESC_BYTE(0x02),                  /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  USB_DESC_BYTE(5),                     /* bFunctionLength.                 */
  USB_DESC_BYTE(0x24),                  /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE(0x06),                  /* bDescriptorSubtype (Union
                                         * Functional Descriptor).          */
  USB_DESC_BYTE(0x00),                  /* bMasterInterface (Communication
                                         * Class Interface).                */
  USB_DESC_BYTE(0x01),                  /* bSlaveInterface0 (Data Class
                                         * Interface).                      */
  /* Endpoint 2 Descriptor.*/
  USB_DESC_ENDPOINT(USBD1_INTERRUPT_REQUEST_EP | 0x80,
                    0x03,               /* bmAttributes (Interrupt).        */
                    0x0008,             /* wMaxPacketSize.                  */
                    0xFF),              /* bInterval.                       */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE(0x01,              /* bInterfaceNumber.                */
                     0x00,              /* bAlternateSetting.               */
                     0x02,              /* bNumEndpoints.                   */
                     0x0A,              /* bInterfaceClass (Data Class
                                         * Interface, CDC section 4.5).     */
                     0x00,              /* bInterfaceSubClass (CDC section
                                         * 4.6).                            */
                     0x00,              /* bInterfaceProtocol (CDC section
                                         * 4.7).                            */
                     0x00),             /* iInterface.                      */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT(USBD1_DATA_AVAILABLE_EP,            /* bEndpointAddress.*/
                    0x02,               /* bmAttributes (Bulk).             */
                    0x0040,             /* wMaxPacketSize.                  */
                    0x00),              /* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT(USBD1_DATA_REQUEST_EP | 0x80,       /* bEndpointAddress.*/
                    0x02,               /* bmAttributes (Bulk).             */
                    0x0040,             /* wMaxPacketSize.                  */
                    0x00)               /* bInterval.                       */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
  sizeof vcom_configuration_descriptor_data,
  vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
  USB_DESC_BYTE(2+2*7),                 /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'N', 0, 'o', 0, 'p', 0, 'e', 0, 'L', 0, 'a', 0, 'b', 0,
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
  USB_DESC_BYTE(2+5*2),                 /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'C', 0, 'h', 0, 'T', 0, 's', 0, 'y', 0,
};

/*
 * Serial Number string.
 */
static const uint8_t vcom_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {
  { sizeof vcom_string0, vcom_string0 },
  { sizeof vcom_string1, vcom_string1 },
  { sizeof vcom_string2, vcom_string2 },
  { sizeof vcom_string3, vcom_string3 }
};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {
  (void)usbp;
  (void)lang;
  switch(dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &vcom_device_descriptor;

  case USB_DESCRIPTOR_CONFIGURATION:
    return &vcom_configuration_descriptor;

  case USB_DESCRIPTOR_STRING:
    if(dindex < 4)
      return &vcom_strings[dindex];
  }
  return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  sduDataReceived,
  0x0040,
  0x0040,
  &ep1instate,
  &ep1outstate,
  2,
  NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  0x0010,
  0x0000,
  &ep2instate,
  NULL,
  1,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
  switch(event) {
  case USB_EVENT_RESET:
    return;

  case USB_EVENT_ADDRESS:
    return;

  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    /* Enables the endpoints specified into the configuration.
     * Note, this callback is invoked from an ISR so I-Class functions
     * must be used.*/
    usbInitEndpointI(usbp, USBD1_DATA_REQUEST_EP, &ep1config);
    usbInitEndpointI(usbp, USBD1_INTERRUPT_REQUEST_EP, &ep2config);

    /* Resetting the state of the CDC subsystem.*/
    sduConfigureHookI(&SDU1);

    chSysUnlockFromISR();
    return;

  case USB_EVENT_SUSPEND:
    chSysLockFromISR();

    /* Disconnection event on suspend.*/
    sduDisconnectI(&SDU1);

    chSysUnlockFromISR();
    return;

  case USB_EVENT_WAKEUP:
    return;

  case USB_EVENT_STALLED:
    return;
  }
  return;
}

/*
 * Handles the Start of Frame event.
 */
static void sof_handler(USBDriver *usbp) {

  (void)usbp;

  osalSysLockFromISR();
  sduSOFHookI(&SDU1);
  osalSysUnlockFromISR();
}

/*
 * USB driver configuration.
 */
static const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  sduRequestsHook,
  sof_handler
};

/*
 * Serial over USB driver configuration.
 */
static const SerialUSBConfig serusbcfg = {
  &USBD1,
  USBD1_DATA_REQUEST_EP,
  USBD1_DATA_AVAILABLE_EP,
  USBD1_INTERRUPT_REQUEST_EP
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
 * Wiegand.
 *===========================================================================*/

void wieg_init(void);
void wieg_send(uint8_t* buf, uint8_t n);
void phex4(uint8_t c);
void phex(uint8_t c);
void phex16(uint32_t c);
void phex24(uint32_t c);
void phex32(uint32_t c);
void wieg_decode_26(uint8_t *buf, uint8_t n);

#if defined(TEENSY30) || defined(TEENSY32)
#define WIEG_IN_DAT0_GPIO TEENSY_PIN6_IOPORT
#define WIEG_IN_DAT0_PORT PORTD
#define WIEG_IN_DAT0_PIN TEENSY_PIN6
#define WIEG_IN_DAT1_GPIO TEENSY_PIN7_IOPORT
#define WIEG_IN_DAT1_PORT PORTD
#define WIEG_IN_DAT1_PIN TEENSY_PIN7

#define WIEG_SHOULD_RECEIVE FALSE
#define WIEG_PINS_MODE PAL_MODE_INPUT_PULLUP
#endif

#if defined(MCHCK)
#define WIEG_IN_DAT0_GPIO GPIOD
#define WIEG_IN_DAT0_PORT PORTD
#define WIEG_IN_DAT0_PIN 1
#define WIEG_IN_DAT1_GPIO GPIOD
#define WIEG_IN_DAT1_PORT PORTD
#define WIEG_IN_DAT1_PIN 0

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG_PINS_MODE PAL_MODE_INPUT_PULLUP
#endif

#if defined(KL27Z)
#define WIEG_IN_DAT0_GPIO GPIOD
#define WIEG_IN_DAT0_PORT PORTD
#define WIEG_IN_DAT0_PIN 1
#define WIEG_IN_DAT1_GPIO GPIOD
#define WIEG_IN_DAT1_PORT PORTD
#define WIEG_IN_DAT1_PIN 0

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG_PINS_MODE PAL_MODE_INPUT_PULLUP
#endif

#if defined(F042)
#define WIEG_IN_DAT0_GPIO GPIOA
#define WIEG_IN_DAT0_PORT PORTA
#define WIEG_IN_DAT0_PIN 0
#define WIEG_IN_DAT1_GPIO GPIOA
#define WIEG_IN_DAT1_PORT PORTA
#define WIEG_IN_DAT1_PIN 1

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG_PINS_MODE PAL_MODE_INPUT_PULLUP
#endif

#define WIEG_PULSE_WIDTH_MIN (US2ST(20))
#define WIEG_PULSE_WIDTH     (US2ST(50))
#define WIEG_PULSE_WIDTH_MAX (US2ST(100))
#define WIEG_PAUSE_WIDTH_MIN (US2ST(200))
#define WIEG_PAUSE_WIDTH     (MS2ST(2))
#define WIEG_PAUSE_WIDTH_MAX (MS2ST(20))
#define WIEG_SAMPLE_WAIT     (US2ST(5))

volatile systime_t wieg_last_pulse_time;
volatile uint8_t wieg_reading_sequence = 0;
uint8_t wieg_buffer[100];
volatile uint8_t wieg_buffer_pos = 0;

static void extcb0(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg_last_pulse_time = chVTGetSystemTimeX();
  wieg_reading_sequence = 1;
  led_blink = 1;
  wieg_buffer[wieg_buffer_pos++] = 0;
  osalSysUnlockFromISR();
}

static void extcb1(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg_last_pulse_time = chVTGetSystemTimeX();
  wieg_reading_sequence = 1;
  led_blink = 1;
  wieg_buffer[wieg_buffer_pos++] = 1;
  osalSysUnlockFromISR();
}

#if defined(TEENSY30) || defined(TEENSY32) || defined(MCHCK) || defined(KL27Z)
static const EXTConfig extcfg = {
  {
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb0, WIEG_IN_DAT0_PORT, WIEG_IN_DAT0_PIN},
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb1, WIEG_IN_DAT1_PORT, WIEG_IN_DAT1_PIN}
  }
};
#elif defined(F042)
static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb0},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};
#endif

#if WIEG_SHOULD_RECEIVE
static THD_WORKING_AREA(waWiegThr, 128);
static THD_FUNCTION(WiegThr, arg) {
  (void)arg;

  while(true) { 
    if((wieg_reading_sequence == 1) && 
        ((chVTGetSystemTime() - wieg_last_pulse_time) >= 2*WIEG_PAUSE_WIDTH_MAX)) {
      // finished reading
      wieg_reading_sequence = 0;
      // print it out
      // uint8_t i;
      // for(i=0; i<wieg_buffer_pos; i++) {
      //   chnPutTimeout(&SDU1, '0'+wieg_buffer[i], TIME_IMMEDIATE);
      // }
      // chnPutTimeout(&SDU1, '\r', TIME_IMMEDIATE);
      // chnPutTimeout(&SDU1, '\n', TIME_IMMEDIATE);
      // ... or line this
      wieg_decode_26(wieg_buffer, wieg_buffer_pos);
      wieg_buffer_pos=0;
    }
  chThdSleep(WIEG_PAUSE_WIDTH);
  }
}
#endif /* WIEG_SHOULD_RECEIVE */

uint8_t wieg_test_buf[26] = {0, 1,1,0,0,1,1,0,0,1,1,0,0, 1,1,0,0,1,1,0,0,1,1,0,0, 1};

/*
 * Wiegand init code
 */
void wieg_init(void) {
  // iqObjectInit(&wiegand_input_queue, wiegand_input_queue_buffer, sizeof(wiegand_input_queue_buffer), wiegand_input_queue_inotify, NULL);
  palSetPadMode(WIEG_IN_DAT0_GPIO, WIEG_IN_DAT0_PIN, WIEG_PINS_MODE);
  palSetPadMode(WIEG_IN_DAT1_GPIO, WIEG_IN_DAT1_PIN, WIEG_PINS_MODE);
#if (WIEG_SHOULD_RECEIVE)
  chThdCreateStatic(waWiegThr, sizeof(waWiegThr), NORMALPRIO, WiegThr, NULL);
  extStart(&EXTD1, &extcfg);
#endif
}

/*
 * Wiegand write
 * - expects bits (zeroes and non-zeroes) in buf
 * - also expects the lines to be pulled up externally
 */
void wieg_send(uint8_t* buf, uint8_t n) {
  uint8_t i;
#if WIEG_SHOULD_RECEIVE
  extChannelDisable(&EXTD1, 0);
  extChannelDisable(&EXTD1, 1);
#endif /* WIEG_SHOULD_RECEIVE */
  osalSysLock();
  palSetPadMode(WIEG_IN_DAT0_GPIO, WIEG_IN_DAT0_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(WIEG_IN_DAT1_GPIO, WIEG_IN_DAT1_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  for(i=0; i<n; i++) {
    if(buf[i]==0) {
      palClearPad(WIEG_IN_DAT0_GPIO, WIEG_IN_DAT0_PIN);
      chThdSleepS(WIEG_PULSE_WIDTH);
      palSetPad(WIEG_IN_DAT0_GPIO, WIEG_IN_DAT0_PIN);
    } else {
      palClearPad(WIEG_IN_DAT1_GPIO, WIEG_IN_DAT1_PIN);
      chThdSleepS(WIEG_PULSE_WIDTH);
      palSetPad(WIEG_IN_DAT1_GPIO, WIEG_IN_DAT1_PIN);
    }
    chThdSleepS(WIEG_PAUSE_WIDTH);
  }
  palSetPadMode(WIEG_IN_DAT0_GPIO, WIEG_IN_DAT0_PIN, WIEG_PINS_MODE);
  palSetPadMode(WIEG_IN_DAT1_GPIO, WIEG_IN_DAT1_PIN, WIEG_PINS_MODE);
  osalSysUnlock();
#if WIEG_SHOULD_RECEIVE
  extChannelEnable(&EXTD1, 0);
  extChannelEnable(&EXTD1, 1);
#endif /* WIEG_SHOULD_RECEIVE */
}

uint8_t one(uint8_t m) {
  if(m==0)
    return 0;
  else
    return 1;
}

void phex4(uint8_t c) {
  chnPutTimeout(&SDU1, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE);
}

void phex(uint8_t c) {
  phex4(c >> 4);
  phex4(c & 15);
}

void phex16(uint32_t c) {
  phex((uint8_t)(c>>8));
  phex((uint8_t)c);
}

void phex24(uint32_t c) {
  phex16((uint32_t)((c>>8)&0xFFFF));
  phex((uint8_t)c);
}

void phex32(uint32_t c) {
  phex16(c>>16);
  phex16(c&0xFFFF);
}

uint8_t wieg_count_ones(uint8_t *buf, uint8_t n) {
  uint8_t i;
  uint8_t count = 0;
  for(i=0; i<n; i++) {
    if(buf[i]!=0)
      count++;
  }
  return(count);
}

void wieg_decode_26(uint8_t *buf, uint8_t n) {
  if(n < 26) {
    return;
  }
  /* check parity */
  if(  ((wieg_count_ones(buf+1,12) +one(buf[0]) ) % 2 == 1) 
    || ((wieg_count_ones(buf+13,12)+one(buf[25])) % 2 == 0) ) {
    return;
  }
  /* collect bits into one number */
  uint32_t msg = 0;
  uint8_t i;
  for(i=1; i<25; i++) {
    msg = (msg<<1) | (buf[i]==0 ? 0 : 1);
  }
  /* hexprint */
  phex24(msg);
  chnPutTimeout(&SDU1, '\r', TIME_IMMEDIATE);
  chnPutTimeout(&SDU1, '\n', TIME_IMMEDIATE);
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
   * Creates the blinker thread.
   */
  chThdCreateStatic(waBlinkThr, sizeof(waBlinkThr), NORMALPRIO, BlinkThr, NULL);

  /* Setup pins for wiegand */
  wieg_init();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

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
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while(true) {
    if((palReadPad(BUTTON_GPIO, BUTTON_PIN) == BUTTON_ACTIVE) && (SDU1.config->usbp->state == USB_ACTIVE)) {
      /* sdWrite(&SDU1, (uint8_t *)"hello world\r\n", 13); */
      // chprintf((BaseSequentialStream *)&SDU1, "Hello world\r\n");
      chnPutTimeout(&SDU1, 'W', TIME_IMMEDIATE);          
      wieg_send(wieg_test_buf, 26);
      led_blink = 1;
      chThdSleepMilliseconds(200);
      /* chnWrite((BaseChannel *)&SDU1, (uint8_t *)"Hello, world\r\n", 14); */
    }

    msg_t charbuf;
    charbuf = chnGetTimeout(&SDU1, TIME_IMMEDIATE);
    if(charbuf != Q_TIMEOUT) {
      chnPutTimeout(&SDU1, (uint8_t)charbuf, TIME_IMMEDIATE);
      if(charbuf == '\r') {
        chnPutTimeout(&SDU1, '\n', TIME_IMMEDIATE);          
      }
    }

    chThdSleepMilliseconds(50);
  }
}
