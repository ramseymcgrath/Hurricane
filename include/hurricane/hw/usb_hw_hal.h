#pragma once
#include <stdint.h>

// Hardware-level USB setup packet
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_hw_setup_packet_t;

// Initialize and run low-level USB hardware
void usb_hw_init(void);
void usb_hw_task(void);
void usb_hw_reset_bus(void);

// Perform control transfers
int usb_hw_send_control_transfer(const usb_hw_setup_packet_t* setup, void* buffer, uint16_t length);
