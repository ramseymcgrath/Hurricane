#ifndef USB_CONTROL_H
#define USB_CONTROL_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

void usb_handle_setup_packet(const usb_setup_packet_t* setup);

#endif // USB_CONTROL_H
