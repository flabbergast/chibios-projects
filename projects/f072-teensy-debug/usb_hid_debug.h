/*
 * Copyright (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 *
 * Based on the work of Guillaume Duc, original licence below.
 * The original license applies to the whole current file.
 */

/*

  Copyright (c) 2014 Guillaume Duc <guillaume@guiduc.org>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#ifndef _USB_HID_DEBUG_H_
#define _USB_HID_DEBUG_H_

#include "ch.h"
#include "hal.h"

/* 
 * Teensy HID debug IN packet size
 *  - this much gets transmitted with every IN packet
 */

#define DEBUG_TX_SIZE 8

/*
 * Teensy HID debug IN endpoint number
 */
#define DEBUG_TX_ENDPOINT 3

/*
 * Teensy HID debug flush time (ms)
 */
#define DEBUG_TX_FLUSH_MS 50

/**
 * @name    Teensy HID debug configuration options
 * @{
 */
/**
 * @brief   Teensy HID debug buffer size.
 * @details Configuration parameter, the buffer size must be a multiple of
 *          the USB data endpoint maximum packet size.
 * @note    The default is 2*DEBUG_TX_SIZE
 */
#if !defined(HID_DEBUG_OUTPUT_BUFFER_SIZE) || defined(__DOXYGEN__)
#define HID_DEBUG_OUTPUT_BUFFER_SIZE     2*DEBUG_TX_SIZE
#endif
/** @} */

/**
 * @brief Teensy HID debug Driver state machine possible states.
 */
typedef enum {
  HIDDEBUG_UNINIT = 0,                   /**< Not initialized.                   */
  HIDDEBUG_STOP = 1,                     /**< Stopped.                           */
  HIDDEBUG_READY = 2                     /**< Ready.                             */
} hid_debug_state_t;

/**
 * @brief   Structure representing a teensy HID debug driver.
 */
typedef struct HIDDebugDriver HIDDebugDriver;

/**
 * @brief   Teensy HID debug Driver configuration structure.
 * @details An instance of this structure must be passed to @p hidDebugStart()
 *          in order to configure and start the driver operations.
 */
typedef struct {
  /**
   * @brief   USB driver to use.
   */
  USBDriver                 *usbp;
  /**
   * @brief   Bulk IN endpoint used for outgoing data transfer.
   */
  usbep_t                   ep_in;
} HIDDebugConfig;

/**
 * @brief   @p Teensy HID debug Driver specific data.
 */
#define _hid_debug_driver_data                                              \
  _base_asynchronous_channel_data                                           \
  /* Driver state.*/                                                        \
  hid_debug_state_t         state;                                          \
  /* Input queue.*/                                                         \
  input_queue_t             iqueue;                                         \
  /* Output queue.*/                                                        \
  output_queue_t            oqueue;                                         \
  /* Input buffer.*/                                                        \
  uint8_t                   *ib;                                          \
  /* Output buffer.*/                                                       \
  uint8_t                   ob[HID_DEBUG_OUTPUT_BUFFER_SIZE];               \
  /* End of the mandatory fields.*/                                         \
  /* Current configuration data.*/                                          \
  const HIDDebugConfig     *config;


/**
 * @brief   @p HIDDebugDrivr specific methods.
 */
#define _hid_debug_driver_methods                                          \
  _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p HIDDebugDriver virtual methods table.
 */
struct HIDDebugDriverVMT {
  _hid_debug_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Teensy HID debug driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding physical
 *          O queue. (I queues are ignored)
 */
struct HIDDebugDriver {
  /** @brief Virtual Methods Table.*/
  const struct HIDDebugDriverVMT *vmt;
  _hid_debug_driver_data
};


/*
 * Teensy HID debug driver structure.
 */
extern HIDDebugDriver HIDD;

/*
 * USB and Teensy HID debug driver start / stop functions
 */
// Initialize the USB driver and bus
void init_usb_driver(void);
// Initialise and start the teensy HID debug driver
void hid_debug_init_start(HIDDebugDriver *hiddp);
// Stop the teensy HID debug driver
void hid_debug_stop(HIDDebugDriver *hiddp);


void usb_debug_flush_output(HIDDebugDriver *hiddp);



//OLD CODE

// Initialize the USB Input/Output queues
//void init_usb_queues(void);

// Putchar over the USB debug
//msg_t usb_debug_putchar(uint8_t c);

// Flush output (send everything immediately)
//void usb_debug_flush_output(void);

// Helper functions from PJRC
//#define pchar(c) usb_debug_putchar(c)
// void print(const char *s);
// void phex(uint8_t c);
// void phex16(uint16_t i);

#endif /* _USB_HID_H_ */
