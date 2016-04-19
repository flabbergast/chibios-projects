/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "hal.h"

#if defined(KL27Z) || defined(KL25Z)
#define SERIAL_DRIVER SD1
#endif

void serial_init(void);

#endif /* SERIAL_H */