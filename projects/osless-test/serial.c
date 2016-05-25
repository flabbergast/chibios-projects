/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#include "hal.h"

#include "serial.h"

#if defined(KL27Z)
// serial: D7/LPUART0_TX D6/LPUART0_RX
#define serial_pins_setup() palSetPadMode(GPIOD, 7, PAL_MODE_ALTERNATIVE_3); palSetPadMode(GPIOD, 6, PAL_MODE_ALTERNATIVE_3)

SerialConfig serial_config = {
  115200,
};
#endif /* KL27Z */

#if defined(KL25Z)
// serial: default board UART0 routed to the debugger: PTA1/RX PTA2/TX
#define serial_pins_setup()

SerialConfig serial_config = {
  115200,
};
#endif /* KL25Z */

#ifdef PROJECT_USE_SERIAL

void serial_init(void) {
  serial_pins_setup();
  sdStart(&SERIAL_DRIVER, &serial_config);
}

#else /* PROJECT_USE_SERIAL */

void serial_init(void) {}

#endif /* PROJECT_USE_SERIAL */