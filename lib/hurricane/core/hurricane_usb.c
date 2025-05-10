#include "hurricane_usb.h"
#include "hw/hurricane_hw_hal.h"
#include "usb/usb_control.h"
#include <stdint.h>
#include <stdio.h>

hurricane_device_t hurricane_devices[MAX_USB_DEVICES] = {0};
uint8_t hurricane_device_count = 0;

#if defined(CPU_LPC55S69JBD100_cm33_core0) && !defined(__APPLE__)
#include "fsl_clock.h"
#include "fsl_power.h"
#endif

void hurricane_task(void) {
    hurricane_hw_poll();
    for (uint8_t i = 0; i < MAX_USB_DEVICES; i++) {
        if (!hurricane_devices[i].is_active && hurricane_hw_device_connected()) {
            hurricane_devices[i].addr = i + 1; // Assign unique address
            hurricane_devices[i].speed = 2; // Placeholder for speed
            hurricane_devices[i].is_active = 1;
            hurricane_device_count++;
            break;
        }
    }
}

hurricane_device_t* hurricane_get_device(uint8_t index) {
    if (index < hurricane_device_count && hurricane_devices[index].is_active) {
        return &hurricane_devices[index];
    }
    return NULL;
}

int hurricane_control_transfer(hurricane_device_t* dev, hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    if (!dev) return -1;
    return hurricane_hw_control_transfer(setup, buffer, length);
}
