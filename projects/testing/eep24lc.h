/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef EEP24LC_H
#define EEP24LC_H

#define I2C_DRIVER I2CD1

#define EEP24LC_ADDR 0x50

#if defined(TEENSY30) || defined(TEENSY32)
/* Teensy 3.0 and 3.2 */
// PTB2/I2C0_SCL
#define I2C_SCL_GPIO TEENSY_PIN19_IOPORT
#define I2C_SCL_PIN  TEENSY_PIN19
#define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
// PTB3/I2C0_SDA
#define I2C_SDA_GPIO TEENSY_PIN18_IOPORT
#define I2C_SDA_PIN  TEENSY_PIN18
#define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#endif

#if defined(WF)
/* Whitefox */
#define I2C_SCL_GPIO GPIOB
#define I2C_SCL_PIN  0
#define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
#define I2C_SDA_GPIO GPIOB
#define I2C_SDA_PIN  1
#define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#endif

#if defined(MCHCK)
/* MCHCK */
#define I2C_SCL_GPIO GPIOB
#define I2C_SCL_PIN  0
#define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
#define I2C_SDA_GPIO GPIOB
#define I2C_SDA_PIN  1
#define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#endif

#if defined(KL27Z)
/* KL27Z breakout */
// #define I2C_SCL_GPIO GPIOE
// #define I2C_SCL_PIN  24
// #define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_5
// #define I2C_SDA_GPIO GPIOE
// #define I2C_SDA_PIN  25
// #define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_5
#define I2C_SCL_GPIO GPIOC
#define I2C_SCL_PIN  8
#define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
#define I2C_SDA_GPIO GPIOC
#define I2C_SDA_PIN  9
#define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#endif

#if defined(KL25Z)
/* Freedom KL25Z board */
// #define I2C_SCL_GPIO GPIOC
// #define I2C_SCL_PIN  1
// #define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
// #define I2C_SDA_GPIO GPIOC
// #define I2C_SDA_PIN  2
// #define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#define I2C_SCL_GPIO GPIOB
#define I2C_SCL_PIN  2
#define I2C_SCL_MODE PAL_MODE_ALTERNATIVE_2
#define I2C_SDA_GPIO GPIOB
#define I2C_SDA_PIN  3
#define I2C_SDA_MODE PAL_MODE_ALTERNATIVE_2
#endif

#if defined(F042)
/* STM32F042 board */
#endif

void eep24lc_i2c_init(void);
msg_t eep24lc_write_byte(uint16_t addr, uint8_t data);
msg_t eep24lc_read_byte(uint16_t addr, uint8_t *result);
msg_t eep24lc_cur_addr_read_byte(uint8_t *result);

#endif /* EEP24LC_H */