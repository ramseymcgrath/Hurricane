#pragma once

#include <stdint.h>
#include "usb/usb_control.h"

typedef struct {
    uint8_t addr;
    uint8_t speed;
    uint8_t is_active;
} hurricane_device_t;

void hurricane_usb_host_init(void);
void hurricane_task(void);
hurricane_device_t* hurricane_get_device(void);

int hurricane_control_transfer(hurricane_device_t* dev, usb_setup_packet_t* setup, void* buffer, uint16_t length);
