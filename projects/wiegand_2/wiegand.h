/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef WIEGAND_H
#define WIEGAND_H

/*===========================================================================
 * Declarations.
 *===========================================================================*/

void wieg_init(void);
void wieg_send(uint8_t* buf, uint8_t n);
bool wieg_is_26(uint8_t *buf, uint8_t n);
uint32_t wieg_decode_26(uint8_t *buf);

uint16_t read_print_mode(void);
void write_print_mode(uint16_t mode);

extern volatile uint8_t led_blink;

extern volatile uint16_t print_mode;

#define MODE_SIGNATURE 0xBE00

#define MODE_ERR (1<<0)
#define MODE_26  (1<<1)
#define MODE_34  (1<<2)

#define MODE_DEBUG   MODE_ERR|MODE_26|MODE_34
#define MODE_DEFAULT MODE_26|MODE_34

#if defined(F042)
/* Address - beginning of the last 1k page (on 32kB MCUs) */
#define FLASH_ADDR 0x08007C00
#endif /* F042 */

/*===========================================================================
 * Pin definitions.
 *===========================================================================*/

#if defined(TEENSY30) || defined(TEENSY32) || defined(MCHCK) || defined(KL27Z)
#define WIEG1_IN_DAT0_CHANNEL 0
#define WIEG1_IN_DAT1_CHANNEL 1
#define WIEG_HAS_2 FALSE
#endif

#if defined(TEENSY30) || defined(TEENSY32)
#define WIEG1_IN_DAT0_GPIO TEENSY_PIN6_IOPORT
#define WIEG1_IN_DAT0_PORT PORTD
#define WIEG1_IN_DAT0_PIN TEENSY_PIN6
#define WIEG1_IN_DAT1_GPIO TEENSY_PIN7_IOPORT
#define WIEG1_IN_DAT1_PORT PORTD
#define WIEG1_IN_DAT1_PIN TEENSY_PIN7

#define WIEG_SHOULD_RECEIVE FALSE
#define WIEG1_PINS_MODE PAL_MODE_INPUT_PULLUP
#define WIEG1_PINS_OUTPUT_MODE PAL_MODE_OUTPUT_PUSHPULL
#endif

#if defined(MCHCK)
#define WIEG1_IN_DAT0_GPIO GPIOD
#define WIEG1_IN_DAT0_PORT PORTD
#define WIEG1_IN_DAT0_PIN 1
#define WIEG1_IN_DAT1_GPIO GPIOD
#define WIEG1_IN_DAT1_PORT PORTD
#define WIEG1_IN_DAT1_PIN 0

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG1_PINS_MODE PAL_MODE_INPUT_PULLUP
#define WIEG1_PINS_OUTPUT_MODE PAL_MODE_OUTPUT_PUSHPULL
#endif

#if defined(KL27Z)
#define WIEG1_IN_DAT0_GPIO GPIOD
#define WIEG1_IN_DAT0_PORT PORTD
#define WIEG1_IN_DAT0_PIN 1
#define WIEG1_IN_DAT1_GPIO GPIOD
#define WIEG1_IN_DAT1_PORT PORTD
#define WIEG1_IN_DAT1_PIN 0

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG1_PINS_MODE PAL_MODE_INPUT_PULLUP
#define WIEG1_PINS_OUTPUT_MODE PAL_MODE_OUTPUT_PUSHPULL
#endif

#if defined(F042)
#define WIEG1_IN_DAT0_GPIO GPIOF
#define WIEG1_IN_DAT0_PORT PORTF
#define WIEG1_IN_DAT0_PIN 0
#define WIEG1_IN_DAT0_EXT EXT_MODE_GPIOF
#define WIEG1_IN_DAT0_CHANNEL 0
#define WIEG1_IN_DAT1_GPIO GPIOF
#define WIEG1_IN_DAT1_PORT PORTF
#define WIEG1_IN_DAT1_PIN 1
#define WIEG1_IN_DAT1_EXT EXT_MODE_GPIOF
#define WIEG1_IN_DAT1_CHANNEL 1

#define WIEG_SHOULD_RECEIVE TRUE
#define WIEG1_PINS_MODE PAL_MODE_INPUT
#define WIEG1_PINS_OUTPUT_MODE PAL_MODE_OUTPUT_OPENDRAIN

#define WIEG_HAS_2 FALSE
#define WIEG2_IN_DAT0_GPIO GPIOA
#define WIEG2_IN_DAT0_PORT PORTA
#define WIEG2_IN_DAT0_PIN 13
#define WIEG2_IN_DAT0_EXT EXT_MODE_GPIOA
#define WIEG2_IN_DAT0_CHANNEL 13
#define WIEG2_IN_DAT1_GPIO GPIOA
#define WIEG2_IN_DAT1_PORT PORTA
#define WIEG2_IN_DAT1_PIN 14
#define WIEG2_IN_DAT1_EXT EXT_MODE_GPIOA
#define WIEG2_IN_DAT1_CHANNEL 14
#define WIEG2_PINS_MODE PAL_MODE_INPUT
#define WIEG2_PINS_OUTPUT_MODE PAL_MODE_OUTPUT_OPENDRAIN
// also need to edit extcfg (lines are channels)
#endif

/*===========================================================================
 * Protocol definitions.
 *===========================================================================*/

#define WIEG_PULSE_WIDTH_MIN (US2ST(20))
#define WIEG_PULSE_WIDTH     (US2ST(50))
#define WIEG_PULSE_WIDTH_MAX (US2ST(100))
#define WIEG_PAUSE_WIDTH_MIN (US2ST(200))
#define WIEG_PAUSE_WIDTH     (MS2ST(2))
#define WIEG_PAUSE_WIDTH_MAX (MS2ST(20))
#define WIEG_SAMPLE_WAIT     (US2ST(5))

/*===========================================================================
 * Output definitions.
 *===========================================================================*/

#define phex4(chn, c) chnPutTimeout(chn, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE)
#define phex(chn, c) phex4(chn, (c>>4)); phex4(chn, (c&15))
#define phex16(chn, c) phex(chn, (uint8_t)(c>>8)); phex(chn, (uint8_t)c)
#define phex24(chn, c) phex16(chn, (uint32_t)((c>>8)&0xFFFF)); phex(chn, (uint8_t)c)
#define phex32(chn, c) phex16(chn, (c>>16)); phex16(chn, c&0xFFFF)
#define pent(chn) chnWriteTimeout(chn, (const uint8_t *)"\r\n", 2, TIME_IMMEDIATE);

#endif /* WIEGAND_H */
