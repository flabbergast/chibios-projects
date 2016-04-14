/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#include "ch.h"
#include "hal.h"

#include "usbcfg.h"
#include "flash.h"
#include "wiegand.h"

/*===========================================================================
 * Global variables.
 *===========================================================================*/

volatile systime_t wieg1_last_pulse_time;
volatile uint8_t wieg1_reading_sequence = 0;
static uint8_t wieg1_buffer[WIEG_BUFFER_SIZE];
volatile uint8_t wieg1_buffer_pos = 0;

volatile uint16_t print_mode;

#if WIEG_HAS_2
volatile systime_t wieg2_last_pulse_time;
volatile uint8_t wieg2_reading_sequence = 0;
static uint8_t wieg2_buffer[WIEG_BUFFER_SIZE];
volatile uint8_t wieg2_buffer_pos = 0;
#endif

/*===========================================================================
 * Read/write mode in flash.
 *===========================================================================*/
uint16_t read_print_mode(void) {
  uint16_t mode;
  mode = flash_read16(FLASH_ADDR);
  if((mode & MODE_SIGNATURE) == MODE_SIGNATURE) {
    return( mode & 0xFF );
  } else {
    return( MODE_DEFAULT );
  }

}

/* Use sparingly */
void write_print_mode(uint16_t mode) {
  osalSysLock();
  flash_unlock();
  flash_erasepage(FLASH_ADDR);
  flash_write16(FLASH_ADDR, (mode&0xFF)|MODE_SIGNATURE );
  flash_lock();
  osalSysUnlock();
}

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
  // led_blink = 1;
  if(wieg1_buffer_pos < WIEG_BUFFER_SIZE-1) {
    wieg1_buffer[wieg1_buffer_pos++] = 0;
  }
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
  // led_blink = 1;
  if(wieg1_buffer_pos < WIEG_BUFFER_SIZE-1) {
    wieg1_buffer[wieg1_buffer_pos++] = 1;
  }
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
  // led_blink = 1;
  if(wieg2_buffer_pos < WIEG_BUFFER_SIZE-1) {
    wieg2_buffer[wieg2_buffer_pos++] = 0;
  }
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
  // led_blink = 1;
  if(wieg2_buffer_pos < WIEG_BUFFER_SIZE-1) {
    wieg2_buffer[wieg2_buffer_pos++] = 1;
  }
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
void wieg_process_message(uint8_t* buf, uint8_t n, uint8_t label) {
  uint8_t i;
  // check if we can decode in one of the protocols
  // if yes, print it out
  if( wieg_is_26(buf, n) ) {
    if(print_mode&(MODE_DEBUG|MODE_26)) {
      chnPutTimeout(&OUTPUT_CHANNEL, label, TIME_IMMEDIATE);
      if(print_mode&MODE_DEBUG) {
        chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)":26:", 4, TIME_IMMEDIATE);
        for(i=0; i<n; i++) {
          chnPutTimeout(&OUTPUT_CHANNEL, '0'+buf[i], TIME_IMMEDIATE);
        }
        chnPutTimeout(&OUTPUT_CHANNEL, ':', TIME_IMMEDIATE);
      }
      led_blink = 1;
      phex24(&OUTPUT_CHANNEL, wieg_decode_26(buf));
      pent(&OUTPUT_CHANNEL);
    }
  } else if( wieg_is_34(buf,n) ) {
    if( print_mode&(MODE_DEBUG|MODE_34) ) {
      chnPutTimeout(&OUTPUT_CHANNEL, label, TIME_IMMEDIATE);
      if(print_mode&MODE_DEBUG) {
        chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)":34:", 4, TIME_IMMEDIATE);
        for(i=0; i<n; i++) {
          chnPutTimeout(&OUTPUT_CHANNEL, '0'+buf[i], TIME_IMMEDIATE);
        }
        chnPutTimeout(&OUTPUT_CHANNEL, ':', TIME_IMMEDIATE);
      }
      led_blink = 1;
      phex32(&OUTPUT_CHANNEL, wieg_decode_34(buf));
      pent(&OUTPUT_CHANNEL);
    }
  } else if( print_mode&(MODE_DEBUG|MODE_ERR) ) {
    // couldn't decode
    chnPutTimeout(&OUTPUT_CHANNEL, label, TIME_IMMEDIATE);
    if(print_mode&MODE_DEBUG) {
      chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)":err:", 5, TIME_IMMEDIATE);
    }
    chnWriteTimeout(&OUTPUT_CHANNEL, (const uint8_t *)"0x", 2, TIME_IMMEDIATE);
    phex(&OUTPUT_CHANNEL,n);
    chnPutTimeout(&OUTPUT_CHANNEL, ':', TIME_IMMEDIATE);
    led_blink = 1;
    for(i=0; i<n; i++) {
      chnPutTimeout(&OUTPUT_CHANNEL, '0'+buf[i], TIME_IMMEDIATE);
    }
    pent(&OUTPUT_CHANNEL);
  }
}

static THD_WORKING_AREA(waWieg1Thr, 128);
static THD_FUNCTION(Wieg1Thr, arg) {
  (void)arg;
  chRegSetThreadName("wieg_recv_1");

  while(true) { 
    if((wieg1_reading_sequence == 1) && 
        ((chVTGetSystemTime() - wieg1_last_pulse_time) >= 2*WIEG_PAUSE_WIDTH_MAX)) {
      // finished reading
      wieg_process_message(wieg1_buffer, wieg1_buffer_pos, '-');
      // start waiting for a new message
      wieg1_reading_sequence = 0;
      wieg1_buffer_pos=0;
    }
    chThdSleep(WIEG_PAUSE_WIDTH_MAX);
  }
}

#if WIEG_HAS_2
static THD_WORKING_AREA(waWieg2Thr, 128);
static THD_FUNCTION(Wieg2Thr, arg) {
  (void)arg;
  chRegSetThreadName("wieg_recv_2");

  while(true) { 
    if((wieg2_reading_sequence == 1) && 
        ((chVTGetSystemTime() - wieg2_last_pulse_time) >= 2*WIEG_PAUSE_WIDTH_MAX)) {
      // finished reading
      wieg_process_message(wieg2_buffer, wieg2_buffer_pos, '+');
      // start waiting for a new message
      wieg2_reading_sequence = 0;
      wieg2_buffer_pos=0;
    }
    chThdSleep(WIEG_PAUSE_WIDTH_MAX);
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
  print_mode = read_print_mode();
#if (WIEG_SHOULD_RECEIVE)
  chThdCreateStatic(waWieg1Thr, sizeof(waWieg1Thr), NORMALPRIO+3, Wieg1Thr, NULL);
#if WIEG_HAS_2
  chThdCreateStatic(waWieg2Thr, sizeof(waWieg2Thr), NORMALPRIO+3, Wieg2Thr, NULL);
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

bool wieg_is_34(uint8_t *buf, uint8_t n) {
  if(n != 34)
    return false;
  /* check parity */
  if(  ((wieg_count_ones(buf+1,16) +one(buf[0]) ) % 2 == 1) 
    || ((wieg_count_ones(buf+17,16)+one(buf[33])) % 2 == 0) ) {
    return false;
  }
  return true;
}

uint32_t wieg_decode_34(uint8_t *buf) {
  /* collect bits into one number */
  uint32_t msg = 0;
  uint8_t i;
  for(i=1; i<33; i++) {
    msg = (msg<<1) | (buf[i]==0 ? 0 : 1);
  }
  return msg;
}
