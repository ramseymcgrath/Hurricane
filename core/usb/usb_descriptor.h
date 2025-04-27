#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

#include <stdint.h>

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

// TODO can add Configuration Descriptor, Interface Descriptor, etc.)

#endif // USB_DESCRIPTOR_H
