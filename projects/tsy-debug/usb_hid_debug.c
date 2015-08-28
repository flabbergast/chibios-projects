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

// Teensy!
// Mac OS-X and Linux automatically load the correct drivers.  On
// Windows, even though the driver is supplied by Microsoft, an
// INF file is needed to load the driver.  These numbers need to
// match the INF file.
#define VENDOR_ID     0x16C0
#define PRODUCT_ID    0x0479

// HID protocol specific constants
#define USB_DESCRIPTOR_HID 0x21
#define USB_DESCRIPTOR_HID_REPORT 0x22
#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09


/*===========================================================================*/
/* Teensy HID debug driver - extends BaseAsynchronousChannel                 */
/*===========================================================================*/

/*
 * Interface implementation.
 */

static size_t write(void *ip, const uint8_t *bp, size_t n) {
  return oqWriteTimeout(&((HIDDebugDriver *)ip)->oqueue, bp,
                        n, TIME_INFINITE);
}

static size_t read(void *ip, uint8_t *bp, size_t n) {
  (void)ip;
  (void)bp;
  (void)n;
  return 0;
}

static msg_t put(void *ip, uint8_t b) {
  return oqPutTimeout(&((HIDDebugDriver *)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t get(void *ip) {
  (void)ip;
  return Q_RESET;
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout) {
  return oqPutTimeout(&((HIDDebugDriver *)ip)->oqueue, b, timeout);
}

static msg_t gett(void *ip, systime_t timeout) {
  (void)ip;
  (void)timeout;
  return Q_RESET;
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t timeout) {
  return oqWriteTimeout(&((HIDDebugDriver *)ip)->oqueue, bp, n, timeout);
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t timeout) {
  (void)ip;
  (void)bp;
  (void)n;
  (void)timeout;
  return 0;
}

static const struct HIDDebugDriverVMT vmt = {
  write, read, put, get,
  putt, gett, writet, readt
};

/*
 * Flush timer code
 */
// object
static virtual_timer_t hid_debug_flush_timer;
// callback
static void hid_debug_flush_cb(void *arg) {
  HIDDebugDriver *hiddp = (HIDDebugDriver *)arg;
  size_t i,n;
  uint8_t buf[DEBUG_TX_SIZE];
  osalSysLockFromISR();

  // check that the states of things are as they're supposed to
  if((usbGetDriverStateI(hiddp->config->usbp) != USB_ACTIVE) ||
     (hiddp->state != HIDDEBUG_READY)) {
    // rearm the timer
    chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);
    osalSysUnlockFromISR();
    return;
  }

  // don't do anything if the queue or has enough stuff in it
  if(((n = oqGetFullI(&hiddp->oqueue)) == 0) || (n >= DEBUG_TX_SIZE)) {
    // rearm the timer
    chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);
    osalSysUnlockFromISR();
    return;
  }

  // there's stuff hanging in the queue - so dequeue and send
  for(i=0; i<n; i++)
    buf[i] = (uint8_t)oqGetI(&hiddp->oqueue);
  for(i=n; i<DEBUG_TX_SIZE; i++)
    buf[i] = 0;
  osalSysUnlockFromISR();
  usbPrepareTransmit(hiddp->config->usbp, hiddp->config->ep_in, buf, DEBUG_TX_SIZE);
  osalSysLockFromISR();
  (void) usbStartTransmitI(hiddp->config->usbp, hiddp->config->ep_in);
  
  // rearm the timer
  chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);
  osalSysUnlockFromISR();
}

/**
 * @brief   Notification of data inserted into the output queue.
 *
 * @param[in] qp        the queue pointer.
 */
static void onotify(io_queue_t *qp) {
  size_t n;
  HIDDebugDriver *hiddp = qGetLink(qp);

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(hiddp->config->usbp) != USB_ACTIVE) ||
      (hiddp->state != HIDDEBUG_READY)) {
    return;
  }

  /* If there is not an ongoing transaction and the output queue contains
     enough data then a new transaction is started.*/
  if (!usbGetTransmitStatusI(hiddp->config->usbp, hiddp->config->ep_in)) {
    if ((n = oqGetFullI(&hiddp->oqueue)) >= DEBUG_TX_SIZE) {
      osalSysUnlock();

      usbPrepareQueuedTransmit(hiddp->config->usbp,
                               hiddp->config->ep_in,
                               &hiddp->oqueue, DEBUG_TX_SIZE);

      osalSysLock();
      (void) usbStartTransmitI(hiddp->config->usbp, hiddp->config->ep_in);
    }
  }
}

/**
 * @brief   Initializes a generic teensy HID debug driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] hiddp     pointer to a @p HIDDebugDriver structure
 *
 * @init
 */
void hidDebugObjectInit(HIDDebugDriver *hiddp) {

  hiddp->vmt = &vmt;
  osalEventObjectInit(&hiddp->event);
  hiddp->state = HIDDEBUG_STOP;
  // TODO: how to make sure the queue is non-existent?
  //hiddp->iqueue = (input_queue_t)NULL;
  hiddp->ib = (uint8_t*)NULL;
  oqObjectInit(&hiddp->oqueue, hiddp->ob, HID_DEBUG_OUTPUT_BUFFER_SIZE, onotify, hiddp);

  // create the flush timer here as well (TODO: need to clean up/integrate!)
  chVTObjectInit(&hid_debug_flush_timer);
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] hiddp     pointer to a @p HIDDebugDriver object
 * @param[in] config    the teensy HID debug driver configuration
 *
 * @api
 */
void hidDebugStart(HIDDebugDriver *hiddp, const HIDDebugConfig *config) {
  USBDriver *usbp = config->usbp;

  osalDbgCheck(hiddp != NULL);

  osalSysLock();
  osalDbgAssert((hiddp->state == HIDDEBUG_STOP) || (hiddp->state == HIDDEBUG_READY),
                "invalid state");
  usbp->in_params[config->ep_in - 1U]   = hiddp;
  hiddp->config = config;
  hiddp->state = HIDDEBUG_READY;
  osalSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] hiddp      pointer to a @p HIDDebugDriver object
 *
 * @api
 */
void hidDebugStop(HIDDebugDriver *hiddp) {
  USBDriver *usbp = hiddp->config->usbp;

  osalDbgCheck(hiddp != NULL);

  osalSysLock();
  osalDbgAssert((hiddp->state == HIDDEBUG_STOP) || (hiddp->state == HIDDEBUG_READY),
                "invalid state");

  /* Driver in stopped state.*/
  usbp->in_params[hiddp->config->ep_in - 1U]   = NULL;
  hiddp->state = HIDDEBUG_STOP;

  /* Stop the flush timer */
  chVTResetI(&hid_debug_flush_timer);

  /* Queues reset in order to signal the driver stop to the application.*/
  chnAddFlagsI(hiddp, CHN_DISCONNECTED);
  iqResetI(&hiddp->oqueue);
  osalOsRescheduleS();
  osalSysUnlock();
}

/**
 * @brief   USB device configured handler.
 *
 * @param[in] hiddp      pointer to a @p HIDDebugDriver object
 *
 * @iclass
 */
void hidDebugConfigureHookI(HIDDebugDriver *hiddp) {
  oqResetI(&hiddp->oqueue);
  chnAddFlagsI(hiddp, CHN_CONNECTED);
  // Start the flush timer
  chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);
}

/*
 * Function used locally in os/hal/src/usb.c for getting descriptors
 * need it here for HID descriptor
 */
static uint16_t get_hword(uint8_t *p) {
  uint16_t hw;
  hw  = (uint16_t)*p++;
  hw |= (uint16_t)*p << 8U;
  return hw;
}

/**
 * @brief   Default requests hook.
 * @details Applications wanting to use the teensy HID debug driver can use
 *          this function as requests hook in the USB configuration.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The hook status.
 * @retval true         Message handled internally.
 * @retval false        Message not handled.
 */
bool hidDebugRequestsHook(USBDriver *usbp) {
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
         * just send some empty packet
         */
        usbSetupTransfer(usbp, NULL, 0, NULL);
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
      return false;
    usbSetupTransfer(usbp, (uint8_t *) dp->ud_string, dp->ud_size, NULL);
    return true;
  }

  return false;
}

/**
 * @brief   Default data transmitted callback.
 * @details The application must use this function as callback for the IN
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void hidDebugDataTransmitted(USBDriver *usbp, usbep_t ep) {
  HIDDebugDriver *hiddp = usbp->in_params[ep - 1U];
  size_t n;

  if (hiddp == NULL) {
    return;
  }

  osalSysLockFromISR();

  // rearm the flush timer
  chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);

  // see if we've transmitted everything
  if((n = oqGetFullI(&hiddp->oqueue)) == 0) {
    chnAddFlagsI(hiddp, CHN_OUTPUT_EMPTY);
  }
  
  /* Check if there's enough data in the queue to send again */
  if(n >= DEBUG_TX_SIZE) {
    /* The endpoint cannot be busy, we are in the context of the callback,
       so it is safe to transmit without a check.*/
    osalSysUnlockFromISR();

    usbPrepareQueuedTransmit(usbp, ep, &hiddp->oqueue, DEBUG_TX_SIZE);

    osalSysLockFromISR();
    (void) usbStartTransmitI(usbp, ep);
  }

  osalSysUnlockFromISR();
}

/*===========================================================================*/
/* Teensy HID debug driver - END OF CODE                                     */
/*===========================================================================*/



/*===========================================================================*/
/* USB Descriptors                                                           */
/*===========================================================================*/

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

  // HID debug Endpoint (IN) Descriptor (7 bytes)
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

/*===========================================================================*/
/* USB driver config structures                                              */
/*===========================================================================*/

// Declare callbacks
static const USBDescriptor * usb_get_descriptor_cb(USBDriver* usbp, uint8_t dtype, uint8_t dindex, uint16_t lang);
static void usb_event_cb(USBDriver * usbp, usbevent_t event);

// State structures
static USBInEndpointState hid_debug_in_ep_state;

// HID debug endpoing initialization structure (only IN)
static const USBEndpointConfig hid_debug_ep_config = {
  USB_EP_MODE_TYPE_INTR,        // Interrupt EP
  NULL,                         // SETUP packet notification callback
  hidDebugDataTransmitted,      // IN notification callback
  NULL,                         // OUT notification callback
  DEBUG_TX_SIZE,                // IN maximum packet size
  0x0000,                       // OUT maximum packet size
  &hid_debug_in_ep_state,       // IN Endpoint state
  NULL,                         // OUT endpoint state
  2,                            // IN multiplier
  NULL                          // SETUP buffer (not a SETUP endpoint)
};

// USB driver configuration
static const USBConfig usbcfg = {
  usb_event_cb,                 // USB events callback
  usb_get_descriptor_cb,        // Device GET_DESCRIPTOR request callback
  hidDebugRequestsHook,         // Requests hook callback
  NULL                          // Start Of Frame callback
};

static const HIDDebugConfig hiddebugcfg = {
  &USBD1,
  DEBUG_TX_ENDPOINT,
};

/*===========================================================================*/
/* USB functions                                                             */
/*===========================================================================*/

/*
 * Handles the GET_DESCRIPTOR callback
 * Returns the proper descriptor
 */
static const USBDescriptor * usb_get_descriptor_cb(USBDriver* usbp, uint8_t dtype, uint8_t dindex, uint16_t lang) {
  (void)usbp;
  (void)lang;

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
      usbInitEndpointI(usbp, DEBUG_TX_ENDPOINT, &hid_debug_ep_config);
      hidDebugConfigureHookI(&HIDD);
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


/*===========================================================================*/
/* init/stop and callable functions                                          */
/*===========================================================================*/

/*
 * Initialize the USB driver
 */
void init_usb_driver(void) {
  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(hiddebugcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(hiddebugcfg.usbp, &usbcfg);
  usbConnectBus(hiddebugcfg.usbp);
}

/*
 * Initialise and start the teensy HID debug driver.
 */
void hid_debug_init_start(HIDDebugDriver *hiddp) {
  hidDebugObjectInit(hiddp);
  hidDebugStart(hiddp, &hiddebugcfg);
}

/*
 * Stop the teensy HID debug driver.
 */
void hid_debug_stop(HIDDebugDriver *hiddp) {
  hidDebugStop(hiddp);
}


void usb_debug_flush_output(HIDDebugDriver *hiddp) {
  size_t n;
  // we'll sleep for a moment to finish any transfers that may be pending already
  // there's a race condition somewhere, maybe because we have 2x buffer
  chThdSleepMilliseconds(2);
  osalSysLock();  
  // check that the states of things are as they're supposed to
  if((usbGetDriverStateI(hiddp->config->usbp) != USB_ACTIVE) ||
     (hiddp->state != HIDDEBUG_READY)) {
    osalSysUnlock();
    return;
  }

  // rearm the timer
  chVTSetI(&hid_debug_flush_timer, MS2ST(DEBUG_TX_FLUSH_MS), hid_debug_flush_cb, hiddp);
    
  // don't do anything if the queue is empty
  if((n = oqGetFullI(&hiddp->oqueue)) == 0) {
    osalSysUnlock();
    return;
  }

  osalSysUnlock();
  // if we don't have enough bytes in the queue, fill with zeroes
  while(n++ < DEBUG_TX_SIZE) {
    oqPut(&hiddp->oqueue, 0);
  }
  // will transmit automatically because of the onotify callback
  // which transmits as soon as the queue has enough
}
