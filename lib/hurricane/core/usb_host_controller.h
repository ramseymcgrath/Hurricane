#pragma once

#include <stdint.h>
#include "core/usb_descriptor.h"
#include "hw/hurricane_hw_hal.h"

typedef enum {
    DEVICE_STATE_DEFAULT,
    DEVICE_STATE_ADDRESS,
    DEVICE_STATE_CONFIGURED,
    DEVICE_STATE_ERROR
} usb_device_state_t;

typedef struct {
    usb_device_state_t state;
    uint8_t device_address;
    usb_device_descriptor_t device_desc; // Store parsed descriptor
} usb_device_t;

void usb_host_init(void);
void usb_host_poll(void);
