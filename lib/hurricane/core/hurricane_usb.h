#pragma once

#include <stdint.h>
#include "hw/hurricane_hw_hal.h"

#define MAX_USB_DEVICES 8

// Define the HID device structure
typedef struct hurricane_hid_device_t {
    // HID class-specific fields
    uint8_t report_id;
    uint8_t protocol;
    uint8_t idle_rate;
    uint16_t report_descriptor_length;
    uint8_t* report_descriptor; // Pointer to report descriptor data
    uint8_t interface_number; // Interface number for this HID device   
} hurricane_hid_device_t;

typedef struct {
    uint8_t addr;
    uint8_t speed;
    uint8_t is_active;
    hurricane_hid_device_t *hid_device;
} hurricane_device_t;

extern hurricane_device_t hurricane_devices[MAX_USB_DEVICES];
extern uint8_t hurricane_device_count;

// Initialization and polling
void hurricane_usb_host_init(void);
void hurricane_task(void);
hurricane_device_t* hurricane_get_device(uint8_t index);

// Control transfer API
int hurricane_control_transfer(hurricane_device_t* dev, hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length);
