/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef OUTPUT_H
#define OUTPUT_H

/*===========================================================================
 * Output helpers.
 *===========================================================================*/
#define phex4(chn, c) chnPutTimeout(chn, c + ((c < 10) ? '0' : 'A' - 10), TIME_IMMEDIATE)
#define phex(chn, c) phex4(chn, (c>>4)); phex4(chn, (c&15))
#define phex16(chn, c) phex(chn, (uint8_t)(c>>8)); phex(chn, (uint8_t)c)
#define phex24(chn, c) phex16(chn, (uint32_t)((c>>8)&0xFFFF)); phex(chn, (uint8_t)c)
#define phex32(chn, c) phex16(chn, (c>>16)); phex16(chn, (c&0xFFFF))
#define pent(chn) chnWriteTimeout(chn, (const uint8_t *)"\r\n", 2, TIME_IMMEDIATE);

#endif /* OUTPUT_H */