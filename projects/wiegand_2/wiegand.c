/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#include "ch.h"
#include "hal.h"

#include "usbcfg.h"
#include "wiegand.h"

/*===========================================================================
 * Global variables.
 *===========================================================================*/

volatile systime_t wieg1_last_pulse_time;
volatile uint8_t wieg1_reading_sequence = 0;
uint8_t wieg1_buffer[100];
volatile uint8_t wieg1_buffer_pos = 0;

#if WIEG_HAS_2
volatile systime_t wieg2_last_pulse_time;
volatile uint8_t wieg2_reading_sequence = 0;
uint8_t wieg2_buffer[100];
volatile uint8_t wieg2_buffer_pos = 0;
#endif

// void phex4(BaseChannel *chn, uint8_t c) {
//   chnPutTimeout(chn, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE);
// }

// #define phex4(chn, c) chnPutTimeout(chn, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE)

// void phex(BaseChannel *chn, uint8_t c) {
//   phex4(chn, c >> 4);
//   phex4(chn, c & 15);
// }

// #define phex(chn, c) phex4(chn, (c>>4)); phex4(chn, (c&15))

// void phex16(BaseChannel *chn, uint32_t c) {
//   phex(chn, (uint8_t)(c>>8));
//   phex(chn, (uint8_t)c);
// }

// #define phex16(chn, c) phex(chn, (uint8_t)(c>>8)); phex(chn, (uint8_t)c)

// void phex24(BaseChannel *chn, uint32_t c) {
//   phex16(chn, (uint32_t)((c>>8)&0xFFFF));
//   phex(chn, (uint8_t)c);
// }

// #define phex24(chn, c) phex16(chn, (uint32_t)((c>>8)&0xFFFF)); phex(chn, (uint8_t)c)

// void phex32(BaseChannel *chn, uint32_t c) {
//   phex16(chn, c>>16);
//   phex16(chn, c&0xFFFF);
// }

// #define phex32(chn, c) phex16(chn, (c>>16)); phex16(chn, c&0xFFFF)

// void pent(BaseChannel *chn) {
//   chnPutTimeout(chn, '\r', TIME_IMMEDIATE);
//   chnPutTimeout(chn, '\n', TIME_IMMEDIATE);
// }

// #define pent(chn) chnPutTimeout(chn, '\r', TIME_IMMEDIATE); chnPutTimeout(chn, '\n', TIME_IMMEDIATE);

/*===========================================================================
 * Interrupt callbacks.
 *===========================================================================*/

static void extcb10(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg1_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg1_last_pulse_time = chVTGetSystemTimeX();
  wieg1_reading_sequence = 1;
  led_blink = 1;
  wieg1_buffer[wieg1_buffer_pos++] = 0;
  osalSysUnlockFromISR();
}

static void extcb11(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg1_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg1_last_pulse_time = chVTGetSystemTimeX();
  wieg1_reading_sequence = 1;
  led_blink = 1;
  wieg1_buffer[wieg1_buffer_pos++] = 1;
  osalSysUnlockFromISR();
}

#if WIEG_HAS_2
static void extcb20(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg2_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg2_last_pulse_time = chVTGetSystemTimeX();
  wieg2_reading_sequence = 1;
  led_blink = 1;
  wieg2_buffer[wieg2_buffer_pos++] = 0;
  osalSysUnlockFromISR();
}

static void extcb21(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  osalSysLockFromISR();
  if( (chVTGetSystemTimeX()-wieg2_last_pulse_time) <= WIEG_PULSE_WIDTH_MIN ) {
    osalSysUnlockFromISR();
    return;
  }
  wieg2_last_pulse_time = chVTGetSystemTimeX();
  wieg2_reading_sequence = 1;
  led_blink = 1;
  wieg2_buffer[wieg2_buffer_pos++] = 1;
  osalSysUnlockFromISR();
}
#endif /* WIEG_HAS_2 */

#if defined(TEENSY30) || defined(TEENSY32) || defined(MCHCK) || defined(KL27Z)
static const EXTConfig extcfg = {
  {
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb10, WIEG1_IN_DAT0_PORT, WIEG1_IN_DAT0_PIN},
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb11, WIEG1_IN_DAT1_PORT, WIEG1_IN_DAT1_PIN},
#if WIEG_HAS_2
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb20, WIEG2_IN_DAT0_PORT, WIEG2_IN_DAT0_PIN},
   {EXT_CH_MODE_FALLING_EDGE|EXT_CH_MODE_AUTOSTART, extcb21, WIEG2_IN_DAT1_PORT, WIEG2_IN_DAT1_PIN},
#endif
  }
};
#elif defined(F042)
static const EXTConfig extcfg = {
  {
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | WIEG1_IN_DAT0_EXT, extcb10},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | WIEG1_IN_DAT1_EXT, extcb11},
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
#if WIEG_HAS_2
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | WIEG2_IN_DAT0_EXT, extcb20}, // 13
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | WIEG2_IN_DAT1_EXT, extcb21}, // 14
#else
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
#endif
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

/*===========================================================================
 * Receive.
 *===========================================================================*/

#if WIEG_SHOULD_RECEIVE
void wieg_process_message(uint8_t* buf, uint8_t n) {
  // check if we can decode in one of the protocols
  // if yes, print it out
  if( wieg_is_26(buf, n) ) {
    chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)"26:", 3, TIME_IMMEDIATE);
    phex24(&OUTPUT_CHANNEL, wieg_decode_26(buf));
    pent(&OUTPUT_CHANNEL);
  } else {
    // couldn't decode
    chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)"err:", 4, TIME_IMMEDIATE);
    uint8_t i;
    for(i=0; i<n; i++) {
      chnPutTimeout(&OUTPUT_CHANNEL, '0'+buf[i], TIME_IMMEDIATE);
    }
    pent(&OUTPUT_CHANNEL);
  }
}

static THD_WORKING_AREA(waWieg1Thr, 128);
static THD_FUNCTION(Wieg1Thr, arg) {
  (void)arg;

  while(true) { 
    if((wieg1_reading_sequence == 1) && 
        ((chVTGetSystemTime() - wieg1_last_pulse_time) >= 2*WIEG_PAUSE_WIDTH_MAX)) {
      // finished reading
      wieg_process_message(wieg1_buffer, wieg1_buffer_pos);
      // start waiting for a new message
      wieg1_reading_sequence = 0;
      wieg1_buffer_pos=0;
    }
    chThdSleep(WIEG_PAUSE_WIDTH);
  }
}

#if WIEG_HAS_2
static THD_WORKING_AREA(waWieg2Thr, 128);
static THD_FUNCTION(Wieg2Thr, arg) {
  (void)arg;

  while(true) { 
    if((wieg2_reading_sequence == 1) && 
        ((chVTGetSystemTime() - wieg2_last_pulse_time) >= 2*WIEG_PAUSE_WIDTH_MAX)) {
      // finished reading
      wieg_process_message(wieg2_buffer, wieg2_buffer_pos);
      // start waiting for a new message
      wieg2_reading_sequence = 0;
      wieg2_buffer_pos=0;
    }
    chThdSleep(WIEG_PAUSE_WIDTH);
  }
}
#endif /* WIEG_HAS_2 */
#endif /* WIEG_SHOULD_RECEIVE */

/*===========================================================================
 * Send.
 *===========================================================================*/

/*
 * Wiegand write
 * - expects bits (zeroes and non-zeroes) in buf
 */
void wieg_send(uint8_t* buf, uint8_t n) {
  uint8_t i;
#if WIEG_SHOULD_RECEIVE
  extChannelDisable(&EXTD1, WIEG1_IN_DAT0_CHANNEL);
  extChannelDisable(&EXTD1, WIEG1_IN_DAT1_CHANNEL);
#endif /* WIEG_SHOULD_RECEIVE */
  osalSysLock();
  palSetPadMode(WIEG1_IN_DAT0_GPIO, WIEG1_IN_DAT0_PIN, WIEG1_PINS_OUTPUT_MODE);
  palSetPadMode(WIEG1_IN_DAT1_GPIO, WIEG1_IN_DAT1_PIN, WIEG1_PINS_OUTPUT_MODE);
  for(i=0; i<n; i++) {
    if(buf[i]==0) {
      palClearPad(WIEG1_IN_DAT0_GPIO, WIEG1_IN_DAT0_PIN);
      chThdSleepS(WIEG_PULSE_WIDTH);
      palSetPad(WIEG1_IN_DAT0_GPIO, WIEG1_IN_DAT0_PIN);
    } else {
      palClearPad(WIEG1_IN_DAT1_GPIO, WIEG1_IN_DAT1_PIN);
      chThdSleepS(WIEG_PULSE_WIDTH);
      palSetPad(WIEG1_IN_DAT1_GPIO, WIEG1_IN_DAT1_PIN);
    }
    chThdSleepS(WIEG_PAUSE_WIDTH);
  }
  palSetPadMode(WIEG1_IN_DAT0_GPIO, WIEG1_IN_DAT0_PIN, WIEG1_PINS_MODE);
  palSetPadMode(WIEG1_IN_DAT1_GPIO, WIEG1_IN_DAT1_PIN, WIEG1_PINS_MODE);
  osalSysUnlock();
  chThdSleep(3*WIEG_PAUSE_WIDTH_MAX);
#if WIEG_SHOULD_RECEIVE
  extChannelEnable(&EXTD1, WIEG1_IN_DAT0_CHANNEL);
  extChannelEnable(&EXTD1, WIEG1_IN_DAT1_CHANNEL);
#endif /* WIEG_SHOULD_RECEIVE */
}

/*===========================================================================
 * Init code.
 *===========================================================================*/

void wieg_init(void) {
  // iqObjectInit(&wiegand_input_queue, wiegand_input_queue_buffer, sizeof(wiegand_input_queue_buffer), wiegand_input_queue_inotify, NULL);
  palSetPadMode(WIEG1_IN_DAT0_GPIO, WIEG1_IN_DAT0_PIN, WIEG1_PINS_MODE);
  palSetPadMode(WIEG1_IN_DAT1_GPIO, WIEG1_IN_DAT1_PIN, WIEG1_PINS_MODE);
#if WIEG_HAS_2
  palSetPadMode(WIEG2_IN_DAT0_GPIO, WIEG2_IN_DAT0_PIN, WIEG2_PINS_MODE);
  palSetPadMode(WIEG2_IN_DAT1_GPIO, WIEG2_IN_DAT1_PIN, WIEG2_PINS_MODE);
#endif /* WIEG_HAS_2 */
#if (WIEG_SHOULD_RECEIVE)
  chThdCreateStatic(waWieg1Thr, sizeof(waWieg1Thr), NORMALPRIO, Wieg1Thr, NULL);
#if WIEG_HAS_2
  chThdCreateStatic(waWieg2Thr, sizeof(waWieg2Thr), NORMALPRIO, Wieg2Thr, NULL);
#endif /* WIEG_HAS_2 */
  extStart(&EXTD1, &extcfg);
#endif /* WIEG_SHOULD_RECEIVE */
}

/*===========================================================================
 * Individual protocols.
 *===========================================================================*/

uint8_t one(uint8_t m) {
  if(m==0)
    return 0;
  else
    return 1;
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

bool wieg_is_26(uint8_t *buf, uint8_t n) {
  if(n != 26)
    return false;
  /* check parity */
  if(  ((wieg_count_ones(buf+1,12) +one(buf[0]) ) % 2 == 1) 
    || ((wieg_count_ones(buf+13,12)+one(buf[25])) % 2 == 0) ) {
    return false;
  }
  return true;
}

uint32_t wieg_decode_26(uint8_t *buf) {
  /* collect bits into one number */
  uint32_t msg = 0;
  uint8_t i;
  for(i=1; i<25; i++) {
    msg = (msg<<1) | (buf[i]==0 ? 0 : 1);
  }
  return msg;
}
