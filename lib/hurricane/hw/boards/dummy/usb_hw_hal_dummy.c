#include "hw/hurricane_hw_hal.h"
#include <stdio.h>

void hurricane_hw_init(void) {
    printf("[stub-hal] hurricane_hw_init()\n");
}

void hurricane_hw_poll(void) {
    // no-op
}

int hurricane_hw_device_connected(void) {
    return 1; // Pretend a device is always connected
}

int hurricane_hw_control_transfer(const hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    printf("[stub-hal] hurricane_hw_control_transfer(): Request=0x%02X\n", setup->bRequest);
    if (buffer && length >= 18) {
        for (int i = 0; i < 18; i++) {
            ((uint8_t*)buffer)[i] = i; // Fake data
        }
    }
    return length;
}

int hurricane_hw_interrupt_in_transfer(uint8_t endpoint, void* buffer, uint16_t length) {
    printf("[stub-hal] hurricane_hw_interrupt_in_transfer(): Endpoint=%u, Length=%u\n", endpoint, length);
    if (buffer && length > 0) {
        // Fill with dummy data for testing
        for (int i = 0; i < length && i < 8; i++) {
            ((uint8_t*)buffer)[i] = i + 0x10; // Fake HID report data
        }
        return 8; // Return a fixed size for testing
    }
    return 0;
}

void hurricane_hw_reset_bus(void) {
    printf("[dummy hal] Bus reset\n");
}