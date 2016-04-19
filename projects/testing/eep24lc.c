/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#include "hal.h"

#include "eep24lc.h"

#if defined(KL27Z) || defined(KL25Z) || defined(TEENSY30) || defined(TEENSY32) || defined(WF) || defined(MCHCK)
static const I2CConfig i2ccfg = {
  100000 // clock
};
#endif

#if defined(F042)
static const I2CConfig i2ccfg = {
};
#endif

// internal communication buffers
static uint8_t tx[3] __attribute__((aligned(2)));
static uint8_t rx[1] __attribute__((aligned(2)));

void eep24lc_i2c_init(void) {
  /* I2C pins */
  palSetPadMode(I2C_SCL_GPIO, I2C_SCL_PIN, I2C_SCL_MODE);
  palSetPadMode(I2C_SDA_GPIO, I2C_SDA_PIN, I2C_SDA_MODE);

  /* start I2C */
  i2cStart(&I2C_DRIVER, &i2ccfg);
  // I2C_DRIVER.i2c->F = 0x1C;
  // try high drive
  // I2C_DRIVER.i2c->C2 |= I2Cx_C2_HDRS;
  // try glitch fixing
  // I2C_DRIVER.i2c->FLT = 4;
}

msg_t eep24lc_write_byte(uint16_t addr, uint8_t data) {
  tx[0] = (uint8_t)(addr>>8);
  tx[1] = (uint8_t)(addr&0xFF);
  tx[2] = data;
  return i2cMasterTransmit(&I2C_DRIVER, EEP24LC_ADDR, tx, 3, rx, 0);
}

msg_t eep24lc_read_byte(uint16_t addr, uint8_t *result) {
  tx[0] = (uint8_t)(addr>>8);
  tx[1] = (uint8_t)(addr&0xFF);
  return i2cMasterTransmit(&I2C_DRIVER, EEP24LC_ADDR, tx, 2, result, 1);
}

msg_t eep24lc_cur_addr_read_byte(uint8_t *result) {
  return i2cMasterReceive(&I2C_DRIVER, EEP24LC_ADDR, result, 1);
}