#pragma once

#include <stdint.h>
#include "core/usb_descriptor.h"
#include "hw/usb_hw_hal.h"
// USB request types
#define USB_REQ_TYPE_STANDARD  (0x00)
#define USB_REQ_TYPE_CLASS     (0x20)
#define USB_REQ_TYPE_VENDOR    (0x40)

// USB request recipients
#define USB_REQ_RECIPIENT_DEVICE    (0x00)
#define USB_REQ_RECIPIENT_INTERFACE (0x01)
#define USB_REQ_RECIPIENT_ENDPOINT  (0x02)

// USB standard requests
#define USB_REQ_GET_DESCRIPTOR      (0x06)
#define USB_REQ_SET_ADDRESS         (0x05)

// Control API
int usb_control_set_address(uint8_t address);
int usb_control_get_device_descriptor(uint8_t address, usb_device_descriptor_t* desc_out);
