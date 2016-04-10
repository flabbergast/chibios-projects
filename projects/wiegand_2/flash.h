/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef _FLASH_H_
#define _FLASH_H_

void flash_unlock(void);
void flash_lock(void);
void flash_erasepage(uint32_t page_addr);
void flash_write16(uint32_t flash_addr, uint16_t data);
uint16_t flash_read16(uint32_t addr);

#endif /* _FLASH_H_ */
