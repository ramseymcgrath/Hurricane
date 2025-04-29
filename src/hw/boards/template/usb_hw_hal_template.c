#include "hurricane/hw/usb_hw_hal.h"
#include <stdio.h>

void usb_hw_init(void) {
    printf("[stub-hal] usb_hw_init()\n");
}

void usb_hw_task(void) {
    // no-op
}

int usb_hw_device_connected(void) {
    return 1; // Pretend a device is always connected
}

int usb_hw_send_setup(const usb_hw_setup_packet_t* setup) {
    printf("[stub-hal] usb_hw_send_setup(): Request=0x%02X\n", setup->bRequest);
    return 0;
}

int usb_hw_receive_control_data(uint8_t* buffer, uint16_t length) {
    printf("[stub-hal] usb_hw_receive_control_data(): Length=%u\n", length);
    if (buffer && length >= 18) {
        for (int i = 0; i < 18; i++) {
            buffer[i] = i; // Fake data
        }
    }
    return length;
}

void usb_hw_reset_bus(void) {
    printf("[dummy hal] Bus reset\n");
}
