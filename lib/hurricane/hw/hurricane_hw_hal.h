// File: lib/hurricane/hw/hurricane_hw_hal.h
#pragma once

#include <stdint.h>

// USB setup packet structure
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} hurricane_usb_setup_packet_t;

// Initialize hardware USB host stack
void hurricane_hw_init(void);

// Run per-loop USB polling or maintenance
void hurricane_hw_poll(void);

// Check if a device is attached and usable
int hurricane_hw_device_connected(void);

// Reset the USB bus (full detach + reattach)
void hurricane_hw_reset_bus(void);

// Perform a USB control transfer (setup stage + optional data stage)
int hurricane_hw_control_transfer(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length
);

// Perform a USB interrupt IN transfer (for HID-like data)
int hurricane_hw_interrupt_in_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length
);
