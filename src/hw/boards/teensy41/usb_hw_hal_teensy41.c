#include "hurricane/hw/usb_hw_hal.h"
#include <USBHost_t36.h>

static USBHost myusb;
static USBDevice* device = NULL;

void usb_hw_init(void) {
    myusb.begin();
}

void usb_hw_task(void) {
    myusb.Task();
    if (!device && myusb.getDeviceCount() > 0) {
        device = myusb.getDevice(0);
    }
}

int usb_hw_device_connected(void) {
    return (device != NULL);
}

int usb_hw_send_control_transfer(const usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    if (!device) return -1;
    return myusb.queue_Control_Transfer(device, (setup_t*)setup, buffer, NULL) ? length : -1;
}
