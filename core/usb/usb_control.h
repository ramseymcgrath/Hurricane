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
void usb_control_set_address(uint8_t address);
void usb_control_get_device_descriptor(uint8_t address);

#endif // USB_CONTROL_H
