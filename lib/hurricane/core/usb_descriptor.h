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

// Standard USB Configuration Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} usb_config_descriptor_t;

// Standard USB Interface Descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_interface_descriptor_t;

// Standard USB Endpoint Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} usb_endpoint_descriptor_t;

// HID Descriptor
typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType2;
    uint16_t wDescriptorLength;
} usb_hid_descriptor_t;

// Function to parse a raw 18-byte device descriptor into a struct
int usb_parse_device_descriptor(const uint8_t* raw, usb_device_descriptor_t* desc);

// Function to parse a configuration descriptor
int usb_parse_config_descriptor(const uint8_t* raw, usb_config_descriptor_t* desc);

// Function to parse an interface descriptor
int usb_parse_interface_descriptor(const uint8_t* raw, usb_interface_descriptor_t* desc);

// Function to parse an endpoint descriptor
int usb_parse_endpoint_descriptor(const uint8_t* raw, usb_endpoint_descriptor_t* desc);

// Function to parse a HID descriptor
int usb_parse_hid_descriptor(const uint8_t* raw, usb_hid_descriptor_t* desc);
