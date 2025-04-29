#include "usb_host_controller.h"
#include "hw/hurricane_hw_hal.h"
#include "usb/usb_control.h"
#include <stdio.h>

static usb_device_t device;

void usb_host_init(void)
{
    device.state = DEVICE_STATE_DEFAULT;
    device.device_address = 0;
    
    hurricane_hw_reset_bus(); // Reset the USB bus
    printf("[host] Bus reset initiated\n");
}

void usb_host_poll(void)
{
    switch (device.state)
    {
        case DEVICE_STATE_DEFAULT:
            printf("[host] Setting device address...\n");
            if (usb_control_set_address(1) != 0) {
                printf("[host] Error setting device address.\n");
                device.state = DEVICE_STATE_ERROR;
                break;
            }
            device.device_address = 1;
            device.state = DEVICE_STATE_ADDRESS;
            break;

        case DEVICE_STATE_ADDRESS:
            printf("[host] Fetching device descriptor...\n");
            if (usb_control_get_device_descriptor(device.device_address, &device.device_desc) != 0) {
                printf("[host] Error fetching device descriptor.\n");
                device.state = DEVICE_STATE_ERROR;
                break;
            }
            device.state = DEVICE_STATE_CONFIGURED;
            break;

        case DEVICE_STATE_CONFIGURED:
            printf("[host] Device enumeration complete.\n");
            printf("  VID: 0x%04X, PID: 0x%04X\n", device.device_desc.idVendor, device.device_desc.idProduct);
            break;

        case DEVICE_STATE_ERROR:
        default:
            printf("[host] Device in error state. Resetting...\n");
            hurricane_hw_reset_bus();
            device.state = DEVICE_STATE_DEFAULT;
            break;
    }
}
