/*
 * (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 * Based on ChibiOS 3.0.1 demo code, license below.
 * Licensed under the Apache License, Version 2.0.
 */

/*
 *  ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
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

#ifndef USBCFG_H
#define USBCFG_H

#define OUTPUT_CHANNEL SDU1

#ifndef INCLUDED_FROM_USBCFG_C

extern const USBConfig usbcfg;
extern SerialUSBConfig serusbcfg;
extern SerialUSBDriver OUTPUT_CHANNEL;

#endif /* INCLUDED_FROM_USBCFG_C */

#endif  /* USBCFG_H */

/** @} */
