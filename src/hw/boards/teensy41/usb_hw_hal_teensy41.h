// src/hw/boards/teensy41/usb_hw_hal_teensy41.h
#pragma once

#include <stdint.h>

// Match the hurricane HAL structure
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_hw_setup_packet_t;

void usb_hw_init(void);
void usb_hw_task(void);
int usb_hw_device_connected(void);
int usb_hw_reset_bus(void);
int usb_hw_send_control_transfer(const usb_hw_setup_packet_t* setup, uint8_t* buffer, uint16_t length);
