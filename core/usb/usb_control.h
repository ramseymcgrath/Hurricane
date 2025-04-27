#ifndef USB_CONTROL_H
#define USB_CONTROL_H

#include <stdint.h>

// USB request types
#define USB_REQ_TYPE_STANDARD  (0x00)
#define USB_REQ_TYPE_CLASS     (0x20)
#define USB_REQ_TYPE_VENDOR    (0x40)

#define USB_REQ_RECIPIENT_DEVICE    (0x00)
#define USB_REQ_RECIPIENT_INTERFACE (0x01)
#define USB_REQ_RECIPIENT_ENDPOINT  (0x02)

// USB standard requests
#define USB_REQ_GET_DESCRIPTOR  (0x06)
#define USB_REQ_SET_ADDRESS     (0x05)
#define USB_REQ_SET_CONFIGURATION (0x09)

// USB descriptor types
#define USB_DESC_TYPE_DEVICE        (1)
#define USB_DESC_TYPE_CONFIGURATION (2)
#define USB_DESC_TYPE_STRING        (3)
#define USB_DESC_TYPE_INTERFACE     (4)
#define USB_DESC_TYPE_ENDPOINT      (5)
#define USB_DESC_TYPE_HID           (0x21)
#define USB_DESC_TYPE_REPORT        (0x22)

// USB setup packet structure
typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// Control functions
void usb_handle_setup_packet(const usb_setup_packet_t* setup);
int usb_control_set_address(uint8_t address);
int usb_control_get_device_descriptor(uint8_t address);

#endif // USB_CONTROL_H
