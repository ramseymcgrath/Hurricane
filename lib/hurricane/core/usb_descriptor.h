#pragma once

#include <stdint.h>

// === Descriptor types (moved here) ===
#define USB_DESC_TYPE_DEVICE        0x01
#define USB_DESC_TYPE_CONFIGURATION 0x02
#define USB_DESC_TYPE_STRING        0x03
#define USB_DESC_TYPE_INTERFACE     0x04
#define USB_DESC_TYPE_ENDPOINT      0x05
#define USB_DESC_TYPE_HID            0x21
#define USB_DESC_TYPE_REPORT         0x22
#define USB_DEVICE_DESCRIPTOR_SIZE  18

// Standard USB Device Descriptor (18 bytes total)
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} usb_device_descriptor_t;

// Function to parse a raw 18-byte device descriptor into a struct
int usb_parse_device_descriptor(const uint8_t* raw, usb_device_descriptor_t* desc);
