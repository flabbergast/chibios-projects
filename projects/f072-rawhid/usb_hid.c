/*
 * Copyright (c) 2015 flabbergast <s3+flabbergast@sdfeu.org>
 *
 * Based on the work of Guillaume Duc, original licence below.
 * The original license applies to the whole current file.
 */

/*
 *
 * Copyright (c) 2014 Guillaume Duc <guillaume@guiduc.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ch.h"
#include "hal.h"

#include "usb_hid.h"

/* The USB driver to use */
#define USB_DRIVER USBD1

/* HID specific constants */
#define USB_DESCRIPTOR_HID 0x21
#define USB_DESCRIPTOR_HID_REPORT 0x22
#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09

/* Endpoints */
#define USBD1_IN_EP  1
#define USBD1_OUT_EP 1

input_queue_t usb_input_queue;
static uint8_t usb_input_queue_buffer[USB_INPUT_QUEUE_BUFFER_SIZE];

output_queue_t usb_output_queue;
static uint8_t usb_output_queue_buffer[USB_OUTPUT_QUEUE_BUFFER_SIZE];

static uint8_t in_report_sequence_number = 0;

/* IN EP1 state */
static USBInEndpointState ep1instate;
/* OUT EP1 state */
static USBOutEndpointState ep1outstate;

/* USB Device Descriptor */
static const uint8_t usb_device_descriptor_data[] = {
  USB_DESC_DEVICE(0x0110,       /* bcdUSB (1.1) */
                  0x00,         /* bDeviceClass (defined in later in interface) */
                  0x00,         /* bDeviceSubClass */
                  0x00,         /* bDeviceProtocol */
                  0x40,         /* bMaxPacketSize (64 bytes) */
                  0x0483,       /* idVendor (ST) */
                  0x5740,       /* idProduct (STM32) */
                  0x0000,       /* bcdDevice */
                  1,            /* iManufacturer */
                  2,            /* iProduct */
                  3,            /* iSerialNumber */
                  1)            /* bNumConfigurations */
};

/* Device Descriptor wrapper */
static const USBDescriptor usb_device_descriptor = {
  sizeof usb_device_descriptor_data,
  usb_device_descriptor_data
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
#define HID_DESCRIPTOR_OFFSET 18
#define HID_DESCRIPTOR_SIZE 9

static const uint8_t hid_configuration_descriptor_data[] = {
  /* Configuration Descriptor (9 bytes) */
  USB_DESC_CONFIGURATION(41,    /* wTotalLength (9+9+9+7+7) */
                         0x01,  /* bNumInterfaces */
                         0x01,  /* bConfigurationValue */
                         0,     /* iConfiguration */
                         0xC0,  /* bmAttributes (self powered, set to 0x80 if not) */
                         50),   /* bMaxPower (100mA) */

  /* Interface Descriptor (9 bytes) */
  USB_DESC_INTERFACE(0x00,      /* bInterfaceNumber */
                     0x00,      /* bAlternateSetting */
                     0x02,      /* bNumEndpoints */
                     0x03,      /* bInterfaceClass: HID */
                     0x00,      /* bInterfaceSubClass: None */
                     0x00,      /* bInterfaceProtocol: None */
                     0),        /* iInterface */

  /* HID descriptor (9 bytes) */
  USB_DESC_BYTE(9),             /* bLength */
  USB_DESC_BYTE(0x21),          /* bDescriptorType (HID class) */
  USB_DESC_BCD(0x0110),         /* bcdHID: HID version 1.1 */
  USB_DESC_BYTE(0x00),          /* bCountryCode */
  USB_DESC_BYTE(0x01),          /* bNumDescriptors */
  USB_DESC_BYTE(0x22),          /* bDescriptorType (report desc) */
  USB_DESC_WORD(34),            /* wDescriptorLength */

  /* Endpoint 1 IN Descriptor (7 bytes) */
  USB_DESC_ENDPOINT(USBD1_IN_EP | 0x80,         /* bEndpointAddress */
                    0x03,       /* bmAttributes (Interrupt) */
                    0x0040,     /* wMaxPacketSize */
                    0x0A),      /* bInterval (10 ms) */
  /* Endpoint 1 OUT Descriptor (7 bytes) */
  USB_DESC_ENDPOINT(USBD1_OUT_EP,       /* bEndpointAddress */
                    0x03,       /* bmAttributes (Interrupt) */
                    0x0040,     /* wMaxPacketSize */
                    0x0A)       /* bInterval (10 ms) */
};

/* Configuration Descriptor wrapper */
static const USBDescriptor hid_configuration_descriptor = {
  sizeof hid_configuration_descriptor_data,
  hid_configuration_descriptor_data
};

/* HID descriptor wrapper */
static const USBDescriptor hid_descriptor = {
  HID_DESCRIPTOR_SIZE,
  &hid_configuration_descriptor_data[HID_DESCRIPTOR_OFFSET]
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
  USB_DESC_BYTE(0x06),          /* Usage Page (vendor-defined) */
  USB_DESC_WORD(0xFF00),
  USB_DESC_BYTE(0x09),          /* Usage (vendor-defined) */
  USB_DESC_BYTE(0x01),
  USB_DESC_BYTE(0xA1),          /* Collection (application) */
  USB_DESC_BYTE(0x01),

  USB_DESC_BYTE(0x09),          /* Usage (vendor-defined) */
  USB_DESC_BYTE(0x01),
  USB_DESC_BYTE(0x15),          /* Logical minimum (0) */
  USB_DESC_BYTE(0x00),
  USB_DESC_BYTE(0x26),          /* Logical maximum (255) */
  USB_DESC_WORD(0x00FF),
  USB_DESC_BYTE(0x75),          /* Report size (8 bits) */
  USB_DESC_BYTE(0x08),
  USB_DESC_BYTE(0x95),          /* Report count (USB_HID_IN_REPORT_SIZE) */
  USB_DESC_BYTE(USB_HID_IN_REPORT_SIZE),
  USB_DESC_BYTE(0x81),          /* Input (Data, Variable, Absolute) */
  USB_DESC_BYTE(0x02),

  USB_DESC_BYTE(0x09),          /* Usage (vendor-defined) */
  USB_DESC_BYTE(0x01),
  USB_DESC_BYTE(0x15),          /* Logical minimum (0) */
  USB_DESC_BYTE(0x00),
  USB_DESC_BYTE(0x26),          /* Logical maximum (255) */
  USB_DESC_WORD(0x00FF),
  USB_DESC_BYTE(0x75),          /* Report size (8 bits) */
  USB_DESC_BYTE(0x08),
  USB_DESC_BYTE(0x95),          /* Report count (USB_HID_OUT_REPORT_SIZE) */
  USB_DESC_BYTE(USB_HID_OUT_REPORT_SIZE),
  USB_DESC_BYTE(0x91),          /* Output (Data, Variable, Absolute) */
  USB_DESC_BYTE(0x02),

  USB_DESC_BYTE(0xC0)           /* End collection */
};

/* HID report descriptor wrapper */
static const USBDescriptor hid_report_descriptor = {
  sizeof hid_report_descriptor_data,
  hid_report_descriptor_data
};

/* U.S. English language identifier */
static const uint8_t usb_string_langid[] = {
  USB_DESC_BYTE(4),             /* bLength */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),         /* bDescriptorType */
  USB_DESC_WORD(0x0409)         /* wLANGID (U.S. English) */
};

/* Vendor string */
static const uint8_t usb_string_vendor[] = {
  USB_DESC_BYTE(38),            /* bLength */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),         /* bDescriptorType */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/* Device Description string */
static const uint8_t usb_string_description[] = {
  USB_DESC_BYTE(48),            /* bLength */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),         /* bDescriptorType */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'R', 0, 'T', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 'H', 0,
  'I', 0, 'D', 0, ' ', 0, 'T', 0, 'e', 0, 's', 0, 't', 0
};

/* Serial Number string (will be filled by the function init_usb_serial_string) */
static uint8_t usb_string_serial[] = {
  USB_DESC_BYTE(50),            /* bLength */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING),         /* bDescriptorType */
  '1', 0, '2', 0, '3', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
  '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
  '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0
};

/* Strings wrappers array */
static const USBDescriptor usb_strings[] = {
  { sizeof usb_string_langid, usb_string_langid }
  ,
  { sizeof usb_string_vendor, usb_string_vendor }
  ,
  { sizeof usb_string_description, usb_string_description }
  ,
  { sizeof usb_string_serial, usb_string_serial }
};

/*
 * Handles the GET_DESCRIPTOR callback
 *
 * Returns the proper descriptor
 */
static const USBDescriptor *
usb_get_descriptor_cb(USBDriver *usbp,
                      uint8_t dtype,
                      uint8_t dindex,
                      uint16_t lang){
  (void)usbp;
  (void)lang;

  switch(dtype) {
  /* Generic descriptors */
  case USB_DESCRIPTOR_DEVICE:   /* Device Descriptor */
    return &usb_device_descriptor;

  case USB_DESCRIPTOR_CONFIGURATION:    /* Configuration Descriptor */
    return &hid_configuration_descriptor;

  case USB_DESCRIPTOR_STRING:   /* Strings */
    if(dindex < 4)
      return &usb_strings[dindex];
    break;

  /* HID specific descriptors */
  case USB_DESCRIPTOR_HID:      /* HID Descriptor */
    return &hid_descriptor;

  case USB_DESCRIPTOR_HID_REPORT:       /* HID Report Descriptor */
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
static void
ep1in_cb(USBDriver *usbp,
         usbep_t ep){
  (void)usbp;
  (void)ep;

  chSysLockFromISR();

  /* Check if there is data to send in the output queue */
  if(chOQGetFullI(&usb_output_queue) >= USB_HID_IN_REPORT_SIZE) {
    chSysUnlockFromISR();
    /* Prepare the transmission */
    usbPrepareQueuedTransmit(&USB_DRIVER, USBD1_IN_EP, &usb_output_queue,
                             USB_HID_IN_REPORT_SIZE);
    chSysLockFromISR();
    usbStartTransmitI(&USB_DRIVER, USBD1_IN_EP);
  }

  chSysUnlockFromISR();
}

/*
 * EP1 OUT callback handler
 *
 * Data (OUT report) have just been received. Check if the input queue
 * is not full and in this case, prepare the reception of the next OUT
 * report.
 */
static void
ep1out_cb(USBDriver *usbp,
          usbep_t ep){
  (void)usbp;
  (void)ep;

  chSysLockFromISR();

  /* Check if there is still some space left in the input queue */
  if(chIQGetEmptyI(&usb_input_queue) >= USB_HID_OUT_REPORT_SIZE) {
    chSysUnlockFromISR();
    /* Prepares the reception of new data */
    usbPrepareQueuedReceive(&USB_DRIVER, USBD1_OUT_EP, &usb_input_queue,
                            USB_HID_OUT_REPORT_SIZE);
    chSysLockFromISR();
    usbStartReceiveI(&USB_DRIVER, USBD1_OUT_EP);
  }

  chSysUnlockFromISR();
}

/* EP1 initialization structure (both IN and OUT) */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_INTR,        /* Interrupt EP */
  NULL,                         /* SETUP packet notification callback */
  ep1in_cb,                     /* IN notification callback */
  ep1out_cb,                    /* OUT notification callback */
  0x0040,                       /* IN maximum packet size */
  0x0040,                       /* OUT maximum packet size */
  &ep1instate,                  /* IN Endpoint state */
  &ep1outstate,                 /* OUT endpoint state */
  2,                            /* IN multiplier */
  NULL                          /* SETUP buffer (not a SETUP endpoint) */
};

/* Handles the USB driver global events */
static void
usb_event_cb(USBDriver *usbp, usbevent_t event){
  switch(event) {
  case USB_EVENT_RESET:
    return;

  case USB_EVENT_ADDRESS:
    return;

  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    /* Enable the endpoints specified into the configuration. */
    usbInitEndpointI(usbp, USBD1_IN_EP, &ep1config);

    /* Start the reception immediately */
    chIQResetI(&usb_input_queue);
    usbPrepareQueuedReceive(&USB_DRIVER, USBD1_OUT_EP, &usb_input_queue,
                            USB_HID_OUT_REPORT_SIZE);
    usbStartReceiveI(&USB_DRIVER, USBD1_OUT_EP);

    chSysUnlockFromISR();
    return;

  case USB_EVENT_SUSPEND:
    return;

  case USB_EVENT_WAKEUP:
    return;

  case USB_EVENT_STALLED:
    return;
  }
}

/* Function used locally in os/hal/src/usb.c for getting descriptors */
/* need it here for HID descriptor */
static uint16_t get_hword(uint8_t *p) {
  uint16_t hw;

  hw = (uint16_t)*p++;
  hw |= (uint16_t)*p << 8U;
  return hw;
}

/* Callback for SETUP request on the endpoint 0 (control) */
static bool
usb_request_hook_cb(USBDriver *usbp){
  const USBDescriptor *dp;

  /* Handle HID class specific requests */
  /* Only GetReport is mandatory for HID devices */
  if((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
    if(usbp->setup[1] == HID_GET_REPORT) {
      /* setup[3] (MSB of wValue) = Report ID (must be 0 as we
       * have declared only one IN report)
       * setup[2] (LSB of wValue) = Report Type (1 = Input, 3 = Feature)
       */
      if((usbp->setup[3] == 0) && (usbp->setup[2] == 1)) {
        struct usb_hid_in_report_s in_report;
        usb_build_in_report(&in_report);
        usbSetupTransfer(usbp, (uint8_t *)&in_report,
                         USB_HID_IN_REPORT_SIZE, NULL);
      }
    }
    if(usbp->setup[1] == HID_SET_REPORT) {
      /* Not implemented (yet) */
    }
  }

  /* Handle the Get_Descriptor Request for HID class (not handled by the default hook) */
  if((usbp->setup[0] == 0x81) && (usbp->setup[1] == USB_REQ_GET_DESCRIPTOR)) {
    dp =
      usbp->config->get_descriptor_cb(usbp, usbp->setup[3], usbp->setup[2],
                                      get_hword(&usbp->setup[4]));
    if(dp == NULL)
      return FALSE;

    usbSetupTransfer(usbp, (uint8_t *)dp->ud_string, dp->ud_size, NULL);
    return TRUE;
  }

  return FALSE;
}

/* USB driver configuration */
static const USBConfig usbcfg = {
  usb_event_cb,                 /* USB events callback */
  usb_get_descriptor_cb,        /* Device GET_DESCRIPTOR request callback */
  usb_request_hook_cb,          /* Requests hook callback */
  NULL                          /* Start Of Frame callback */
};

/*
 * Notification of data removed from the input queue
 *
 * If there is sufficient space in the input queue to receive a new
 * OUT report from the PC, prepare the reception of the next OUT
 * report
 */
static void
usb_input_queue_inotify(io_queue_t *qp){
  (void)qp;

  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    return;

  if(chIQGetEmptyI(&usb_input_queue) >= USB_HID_OUT_REPORT_SIZE) {
    chSysUnlock();
    usbPrepareQueuedReceive(&USB_DRIVER, USBD1_OUT_EP, &usb_input_queue,
                            USB_HID_OUT_REPORT_SIZE);

    chSysLock();
    usbStartReceiveI(&USB_DRIVER, USBD1_OUT_EP);
  }
}


/*
 * Notification of data inserted into the output queue
 *
 * If the transmission is not active, prepare the transmission.
 */
static void usb_output_queue_onotify(io_queue_t *qp) {
  (void)qp;

  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    return;

  if(!usbGetTransmitStatusI(&USB_DRIVER, USBD1_IN_EP)
     && (chOQGetFullI(&usb_output_queue) >= USB_HID_IN_REPORT_SIZE)) {
    chSysUnlock();

    usbPrepareQueuedTransmit(&USB_DRIVER, USBD1_IN_EP, &usb_output_queue,
                             USB_HID_IN_REPORT_SIZE);

    chSysLock();
    usbStartTransmitI(&USB_DRIVER, USBD1_IN_EP);
  }
}


/*
 * Initialize the USB input and output queues
 */
void init_usb_queues(void) {
  iqObjectInit(&usb_input_queue, usb_input_queue_buffer,
               sizeof(usb_input_queue_buffer), usb_input_queue_inotify, NULL);

  oqObjectInit(&usb_output_queue, usb_output_queue_buffer,
               sizeof(usb_output_queue_buffer), usb_output_queue_onotify, NULL);
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
 * Queue a IN report to be sent
 */
int usb_send_hid_report(struct usb_hid_in_report_s *report) {
  int res;

  chSysLock();
  if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) {
    chSysUnlock();
    return 0;
  }

  res = chOQGetEmptyI(&usb_output_queue);
  chSysUnlock();

  if(res > USB_HID_IN_REPORT_SIZE) {
    chOQWriteTimeout(&usb_output_queue, (uint8_t *)report,
                     USB_HID_IN_REPORT_SIZE, TIME_INFINITE);

    /* TODO : check error condition */

    return 1;
  } else
    return 0;
}

/*
 * Prepare an IN report
 */
void
usb_build_in_report(struct usb_hid_in_report_s *report){
  report->sequence_number = in_report_sequence_number++;
  report->wkup_pb_value = palReadPad(GPIOA, GPIOA_BUTTON);
}
