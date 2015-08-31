/*
 * (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 *
 * Based on the work of Guillaume Duc, original licence below.
 * The original license applies to the whole current file.
 *
 * Some code also based on the USB Serial implementation
 * in ChibiOS 3.0.1, see the license in ChibiOS.
 *
 * Finally, the print helpers are originally from
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2008 PJRC.COM, LLC
 *  - see the license towards the end of this file.
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

#include "ch.h"
#include "hal.h"

#include "usb_hid_debug.h"

// The USB driver to use
#define USB_DRIVER USBD1

// Teensy!
// Mac OS-X and Linux automatically load the correct drivers.  On
// Windows, even though the driver is supplied by Microsoft, an
// INF file is needed to load the driver.  These numbers need to
// match the INF file.
#define VENDOR_ID     0x16C0
#define PRODUCT_ID    0x0479

// you might want to change the buffer size, up to 64 bytes.
// The host reserves your bandwidth because this is an interrupt
// endpoint, so it won't be available to other interrupt or isync
// endpoints in other devices on the bus.
#define DEBUG_TX_ENDPOINT  3
#define DEBUG_TX_SIZE 5

// Number of IN reports that can be stored inside the output queue
#define USB_OUTPUT_QUEUE_CAPACITY 2

#define USB_OUTPUT_QUEUE_BUFFER_SIZE (USB_OUTPUT_QUEUE_CAPACITY * DEBUG_TX_SIZE)

// HID specific constants
#define USB_DESCRIPTOR_HID 0x21
#define USB_DESCRIPTOR_HID_REPORT 0x22
#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09

output_queue_t usb_output_queue;
static uint8_t usb_output_queue_buffer[USB_OUTPUT_QUEUE_BUFFER_SIZE];

// IN DEBUG_TX_EP state
static USBInEndpointState ep1instate;

// USB Device Descriptor
static const uint8_t usb_device_descriptor_data[] = {
  USB_DESC_DEVICE(0x0200,      // bcdUSB (1.1)
                  0,           // bDeviceClass (defined in later in interface)
                  0,           // bDeviceSubClass
                  0,           // bDeviceProtocol
                  64,          // bMaxPacketSize (64 bytes) (the driver didn't work with 32)
                  VENDOR_ID,   // idVendor
                  PRODUCT_ID,  // idProduct
                  0x0100,      // bcdDevice
                  1,           // iManufacturer
                  2,           // iProduct
                  3,           // iSerialNumber
                  1)           // bNumConfigurations
};

// Device Descriptor wrapper
static const USBDescriptor usb_device_descriptor = {
  sizeof usb_device_descriptor_data,
  usb_device_descriptor_data
};

/*
 * HID Report Descriptor
 *
 * This is the description of the format and the content of the
 * different IN or/and OUT reports that your application can
 * receive/send
 *
 * See "Device Class Definition for Human Interface Devices (HID)"
 * (http://www.usb.org/developers/hidpage/HID1_11.pdf) for the
 * detailed descrition of all the fields
 */
static const uint8_t hid_report_descriptor_data[] = {
  0x06, 0x31, 0xFF, // Usage Page 0xFF31 (vendor defined)
  0x09, 0x74,       // Usage 0x74
  0xA1, 0x53,       // Collection 0x53
  0x75, 0x08,       // report size = 8 bits
  0x15, 0x00,       // logical minimum = 0
  0x26, 0xFF, 0x00, // logical maximum = 255
  0x95, DEBUG_TX_SIZE,  // report count
  0x09, 0x75,       // usage
  0x81, 0x02,       // Input (array)
  0xC0              // end collection
};

// HID report descriptor wrapper
static const USBDescriptor hid_report_descriptor = {
  sizeof hid_report_descriptor_data,
  hid_report_descriptor_data
};

/*
 * Configuration Descriptor tree for a HID device
 *
 * The HID Specifications version 1.11 require the following order:
 * - Configuration Descriptor
 * - Interface Descriptor
 * - HID Descriptor
 * - Endpoints Descriptors
 */
#define HID_DESCRIPTOR_OFFSET 9+9
#define HID_DESCRIPTOR_SIZE 9

static const uint8_t hid_configuration_descriptor_data[] = {
  // Configuration Descriptor (9 bytes)
  USB_DESC_CONFIGURATION(9+9+9+7, // wTotalLength
                         1,    // bNumInterfaces
                         1,    // bConfigurationValue
                         0,    // iConfiguration
                         0xC0, // bmAttributes (self powered, set to 0x80 if not)
                         50),  // bMaxPower (100mA)

  // Interface Descriptor (9 bytes)
  USB_DESC_INTERFACE(0,        // bInterfaceNumber
                     0,        // bAlternateSetting
                     1,        // bNumEndpoints
                     0x03,     // bInterfaceClass: HID
                     0x00,     // bInterfaceSubClass: None
                     0x00,     // bInterfaceProtocol: None
                     0),       // iInterface

  // HID descriptor (9 bytes)
  USB_DESC_BYTE(9),            // bLength
  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
  USB_DESC_BYTE(0),            // bCountryCode
  USB_DESC_BYTE(1),            // bNumDescriptors
  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
  USB_DESC_WORD(sizeof(hid_report_descriptor_data)), // wDescriptorLength

  // Endpoint 1 IN Descriptor (7 bytes)
  USB_DESC_ENDPOINT(DEBUG_TX_ENDPOINT | 0x80,  // bEndpointAddress
                    0x03,      // bmAttributes (Interrupt)
                    DEBUG_TX_SIZE, // wMaxPacketSize
                    1)         // bInterval
};

// Configuration Descriptor wrapper
static const USBDescriptor hid_configuration_descriptor = {
  sizeof hid_configuration_descriptor_data,
  hid_configuration_descriptor_data
};

// HID descriptor wrapper
static const USBDescriptor hid_descriptor = {
  HID_DESCRIPTOR_SIZE,
  &hid_configuration_descriptor_data[HID_DESCRIPTOR_OFFSET]
};

// U.S. English language identifier
static const uint8_t usb_string_langid[] = {
  USB_DESC_BYTE(4),                        // bLength
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),    // bDescriptorType
  USB_DESC_WORD(0x0409)                    // wLANGID (U.S. English)
};

// Vendor string = manufacturer
static const uint8_t usb_string_vendor[] = {
  USB_DESC_BYTE(38),                       // bLength
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),    // bDescriptorType
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

// Device Description string = product
static const uint8_t usb_string_description[] = {
  USB_DESC_BYTE(48),           // bLength
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),    // bDescriptorType
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'R', 0, 'T', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 'H', 0,
  'I', 0, 'D', 0, ' ', 0, 'T', 0, 'e', 0, 's', 0, 't', 0
};

// Serial Number string (will be filled by the function init_usb_serial_string)
static uint8_t usb_string_serial[] = {
  USB_DESC_BYTE(22),                       // bLength
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),    // bDescriptorType
  '0', 0, 'x', 0, 'D', 0, 'E', 0, 'A', 0, 'D', 0, 'B', 0, 'E', 0, 'E', 0, 'F', 0
};

// Strings wrappers array
static const USBDescriptor usb_strings[] = {
  {sizeof usb_string_langid, usb_string_langid}
  ,
  {sizeof usb_string_vendor, usb_string_vendor}
  ,
  {sizeof usb_string_description, usb_string_description}
  ,
  {sizeof usb_string_serial, usb_string_serial}
};

/*
 * Handles the GET_DESCRIPTOR callback
 *
 * Returns the proper descriptor
 */
static const USBDescriptor * usb_get_descriptor_cb(USBDriver __attribute__ ((__unused__)) * usbp,
                       uint8_t dtype,
                       uint8_t dindex,
                       uint16_t __attribute__ ((__unused__)) lang) {
  switch(dtype) {
    // Generic descriptors
    case USB_DESCRIPTOR_DEVICE: // Device Descriptor
      return &usb_device_descriptor;
    case USB_DESCRIPTOR_CONFIGURATION:  // Configuration Descriptor
      return &hid_configuration_descriptor;
    case USB_DESCRIPTOR_STRING: // Strings
      if (dindex < 4)
        return &usb_strings[dindex];
      break;

      // HID specific descriptors
    case USB_DESCRIPTOR_HID:    // HID Descriptor
      return &hid_descriptor;
    case USB_DESCRIPTOR_HID_REPORT:     // HID Report Descriptor
      return &hid_report_descriptor;
  }
  return NULL;
}

/*
 * EP1 IN callback handler
 *
 * Data (IN report) have just been sent to the PC. Check if there are
 * remaining reports to be sent in the output queue and in this case,
 * schedule the transmission
 */
static void ep1in_cb(USBDriver __attribute__ ((__unused__)) * usbp,
          usbep_t __attribute__ ((__unused__)) ep) {
  osalSysLockFromISR();

  // Check if there is data to send in the output queue
  if(chOQGetFullI(&usb_output_queue) >= DEBUG_TX_SIZE) {
    osalSysUnlockFromISR();
    // Prepare the transmission
    usbPrepareQueuedTransmit(&USB_DRIVER, DEBUG_TX_ENDPOINT, &usb_output_queue, DEBUG_TX_SIZE);
    osalSysLockFromISR();
    usbStartTransmitI(&USB_DRIVER, DEBUG_TX_ENDPOINT);
  }

  osalSysUnlockFromISR();
}

// EP1 initialization structure (both IN and OUT)
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_INTR,        // Interrupt EP
  NULL,                         // SETUP packet notification callback
  ep1in_cb,                     // IN notification callback
  NULL,                         // OUT notification callback
  DEBUG_TX_SIZE,                // IN maximum packet size
  0x0000,                       // OUT maximum packet size
  &ep1instate,                  // IN Endpoint state
  NULL,                         // OUT endpoint state
  2,                            // IN multiplier
  NULL                          // SETUP buffer (not a SETUP endpoint)
};

// Handles the USB driver global events
static void usb_event_cb(USBDriver * usbp, usbevent_t event) {
  switch(event) {
    case USB_EVENT_RESET:
      return;
    case USB_EVENT_ADDRESS:
      return;
    case USB_EVENT_CONFIGURED:
      osalSysLockFromISR();
      // Enable the endpoints specified into the configuration.
      usbInitEndpointI(usbp, DEBUG_TX_ENDPOINT, &ep1config);
      osalSysUnlockFromISR();
      return;
    case USB_EVENT_SUSPEND:
      return;
    case USB_EVENT_WAKEUP:
      return;
    case USB_EVENT_STALLED:
      return;
  }
}

// Function used locally in os/hal/src/usb.c for getting descriptors
// need it here for HID descriptor
static uint16_t get_hword(uint8_t *p) {
  uint16_t hw;

  hw  = (uint16_t)*p++;
  hw |= (uint16_t)*p << 8U;
  return hw;
}

// Callback for SETUP request on the endpoint 0 (control)
static bool usb_request_hook_cb(USBDriver * usbp) {
  const USBDescriptor *dp;

  // Handle HID class specific requests
  // Only GetReport is mandatory for HID devices
  if((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
    if(usbp->setup[1] == HID_GET_REPORT) {
      /* setup[3] (MSB of wValue) = Report ID (must be 0 as we
       * have declared only one IN report)
       * setup[2] (LSB of wValue) = Report Type (1 = Input, 3 = Feature)
       */
      if((usbp->setup[3] == 0) && (usbp->setup[2] == 1)) {
        /* When do we get requests like this anyway?
         * (Doing it over ENDPOINT0)
         * just send some random junk
         */
        usbSetupTransfer (usbp, usb_output_queue.q_buffer, DEBUG_TX_SIZE, NULL);
      }
    }
    if(usbp->setup[1] == HID_SET_REPORT) {
        // Not implemented (yet)
    }
  }

  // Handle the Get_Descriptor Request for HID class (not handled by the default hook)
  if((usbp->setup[0] == 0x81) && (usbp->setup[1] == USB_REQ_GET_DESCRIPTOR)) {
    dp = usbp->config->get_descriptor_cb (usbp, usbp->setup[3], usbp->setup[2], get_hword(&usbp->setup[4]));
    if(dp == NULL)
      return FALSE;
    usbSetupTransfer(usbp, (uint8_t *) dp->ud_string, dp->ud_size, NULL);
    return TRUE;
  }

  return FALSE;
}

// USB driver configuration
static const USBConfig usbcfg = {
  usb_event_cb,                 // USB events callback
  usb_get_descriptor_cb,        // Device GET_DESCRIPTOR request callback
  usb_request_hook_cb,          // Requests hook callback
  NULL                          // Start Of Frame callback
};

/*
 * Notification of data inserted into the output queue
 *
 * If the transmission is not active, prepare the transmission.
 */
static void usb_output_queue_onotify(io_queue_t * qp) {
  (void)qp;

  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    return;

  if(!usbGetTransmitStatusI(&USB_DRIVER, DEBUG_TX_ENDPOINT)
      && (chOQGetFullI(&usb_output_queue) >= DEBUG_TX_SIZE)) {
    osalSysUnlock();

    usbPrepareQueuedTransmit(&USB_DRIVER, DEBUG_TX_ENDPOINT, &usb_output_queue, DEBUG_TX_SIZE);

    osalSysLock();
    usbStartTransmitI(&USB_DRIVER, DEBUG_TX_ENDPOINT);
  }
}


/*
 * Initialize the USB input and output queues
 */
void init_usb_queues(void) {
  oqObjectInit(&usb_output_queue, usb_output_queue_buffer, sizeof(usb_output_queue_buffer), usb_output_queue_onotify, NULL);
}


/*
 * Initialize the USB driver
 */
void init_usb_driver(void) {
  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(&USB_DRIVER);
  chThdSleepMilliseconds(1500);
  usbStart(&USB_DRIVER, &usbcfg);
  usbConnectBus(&USB_DRIVER);
}

/*
 * Queue a char to be sent over the USB
 */
msg_t usb_debug_putchar(uint8_t c) {
  // see if USB is up and what's the room in the queue
  osalSysLock();
  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) {
    osalSysUnlock();
    return 0;
  }
  osalSysUnlock();
  // should get suspended and wait if the queue is full
  return(chOQPut(&usb_output_queue, c));
}

/*
 * Flush the output queue, send its contents immediately.
 */
void usb_debug_flush_output(void) {
  size_t bytes_in_queue;
  // we'll sleep for a moment to finish any transfers that may be pending already
  // there's a race condition somewhere, maybe because we have 2x buffer
  chThdSleepMilliseconds(2);
  osalSysLock();
  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) {
    osalSysUnlock();
    return;
  }
  bytes_in_queue = chOQGetFullI(&usb_output_queue);
  osalSysUnlock();
  // if we don't have enough bytes in the queue, fill with zeroes
  while(bytes_in_queue++ < DEBUG_TX_SIZE) {
    chOQPutTimeout(&usb_output_queue, 0, TIME_INFINITE);
  }
  // will transmit automatically because of the onotify callback
  // which transmits as soon as the queue has enough
}


/* Very basic print functions, intended to be used with usb_debug_only.c
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2008 PJRC.COM, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Version 1.0: Initial Release
// Version 1.1: 2015 flabbergast: remove pgm stuff, doesn't apply to ARM

void print(const char *s) {
	char c;
	while (1){
		c = *s++;
		if (!c) break;
		if (c == '\n') usb_debug_putchar('\r');
		usb_debug_putchar(c);
	}
}

void phex1(uint8_t c) {
	usb_debug_putchar(c + ((c < 10) ? '0' : 'A' - 10));
}

void phex(uint8_t c) {
	phex1(c >> 4);
	phex1(c & 15);
}

void phex16(uint16_t i) {
	phex(i >> 8);
	phex(i);
}

