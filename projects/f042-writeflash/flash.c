/*
 * (c) 2016 flabbergast <s3+flabbergast@sdfeu.org>
 * Licensed under the Apache License, Version 2.0.
 */

/*
 * Most of the code is from STM32F0x2 datasheet examples.
 */

/* WARNING WARNING WARNING
 *
 * This code is *dangerous*! It can potentially damage the firmware.
 *
 * The flash has a relatively low guaranteed number of erase operations
 * (on the order of thousands).
 *
 * I would recommend calling these with IRQs and any other possible
 * interruptions disabled, so as not to interrupt the operations,
 * as flash access during them can generate HardFaults.
 */

#include "ch.h"
#include "hal.h"

void flash_unlock(void) {
	/* (1) Wait till no operation is on going */
  /* (2) Check that the Flash is unlocked */
  /* (3) Perform unlock sequence */
  while((FLASH->SR & FLASH_SR_BSY) != 0) { /* (1) */
    /* For robust implementation, add here time-out management */
  }
  if((FLASH->CR & FLASH_CR_LOCK) != 0) { /* (2) */
    FLASH->KEYR = FLASH_KEY1; /* (3) */
    FLASH->KEYR = FLASH_KEY2;
  }
}

void flash_lock(void) {
  FLASH->CR |= FLASH_CR_LOCK;
}

void flash_erasepage(uint32_t page_addr) {
  /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
  /* (2) Program the FLASH_AR register to select a page to erase */
  /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
  /* (4) Wait until the BSY bit is reset in the FLASH_SR register */
  /* (5) Check the EOP flag in the FLASH_SR register */
  /* (6) Clear EOP flag by software by writing EOP at 1 */
  /* (7) Reset the PER Bit to disable the page erase */
  FLASH->CR |= FLASH_CR_PER; /* (1) */
  FLASH->AR = page_addr; /* (2) */
  FLASH->CR |= FLASH_CR_STRT; /* (3) */
  while((FLASH->SR & FLASH_SR_BSY) != 0) { /* (4) */ 
    /* For robust implementation, add here time-out management */
  }
  if((FLASH->SR & FLASH_SR_EOP) != 0) /* (5) */ {
    FLASH->SR |= FLASH_SR_EOP; /* (6)*/
  } else {
    /* Manage the error cases */
  }
  FLASH->CR &= ~FLASH_CR_PER; /* (7) */
}

void flash_write16(uint32_t flash_addr, uint16_t data) {
  /* (1) Set the PG bit in the FLASH_CR register to enable programming */
  /* (2) Perform the data write (half-word) at the desired address */
  /* (3) Wait until the BSY bit is reset in the FLASH_SR register */
  /* (4) Check the EOP flag in the FLASH_SR register */
  /* (5) clear it by software by writing it at 1 */
  /* (6) Reset the PG Bit to disable programming */
  FLASH->CR |= FLASH_CR_PG; /* (1) */
  *(__IO uint16_t*)(flash_addr) = data; /* (2) */
  while((FLASH->SR & FLASH_SR_BSY) != 0) /* (3) */ {
    /* For robust implementation, add here time-out management */
  }
  if((FLASH->SR & FLASH_SR_EOP) != 0) /* (4) */ {
    FLASH->SR |= FLASH_SR_EOP; /* (5) */
  } else {
    /* Manage the error cases */
  }
  FLASH->CR &= ~FLASH_CR_PG; /* (6) */
}

uint16_t flash_read16(uint32_t addr) {
  return *(uint16_t *)(addr);
}
