#include "hurricane_usb.h"
#include "hw/common/usb_hw_hal.h"
#include <stdint.h>

static hurricane_device_t hurricane_dev = {0};

void hurricane_usb_host_init(void) {
    usb_hw_init();
}

void hurricane_task(void) {
    usb_hw_task();

    if (!hurricane_dev.is_active && usb_hw_device_connected()) {
        hurricane_dev.addr = 1; // placeholder
        hurricane_dev.speed = 2; // placeholder (2 = High Speed USB)
        hurricane_dev.is_active = 1;
    }
}

hurricane_device_t* hurricane_get_device(void) {
    if (hurricane_dev.is_active) return &hurricane_dev;
    return NULL;
}

int hurricane_control_transfer(hurricane_device_t* dev, usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    if (!dev) return -1;
    return usb_hw_send_control_transfer(setup, buffer, length);
}
