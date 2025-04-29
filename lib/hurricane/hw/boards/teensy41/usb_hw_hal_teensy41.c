// src/hw/boards/teensy41/usb_hw_hal_teensy41.c

#include "hw/usb_hw_hal.h"
#if defined(ARDUINO_TEENSY41)
#include <USBHost_t36.h>


static USBHost myusb;
static USBDevice *dev = nullptr;
static USBHub hub1(myusb);
static USBHub hub2(myusb);
static USBHub hub3(myusb);

// We'll cache the setup packet here
static usb_hw_setup_packet_t last_setup_packet;

void usb_hw_init(void)
{
    myusb.begin();
}

void usb_hw_task(void)
{
    myusb.Task();
}

int usb_hw_device_connected(void)
{
    if (dev == nullptr) {
        dev = myusb.getDevice();
        if (dev && dev->address > 0) {
            return 1;
        }
    }
    return dev != nullptr;
}

int usb_hw_reset_bus(void)
{
    dev = nullptr;
    myusb.end();
    delay(100);
    myusb.begin();
    return 0;
}

int usb_hw_send_control_transfer(const usb_hw_setup_packet_t* setup, uint8_t* buffer, uint16_t length)
{
    if (!dev) {
        return -1;
    }

    uint8_t bmRequestType = setup->bmRequestType;
    uint8_t bRequest = setup->bRequest;
    uint16_t wValue = setup->wValue;
    uint16_t wIndex = setup->wIndex;
    uint16_t wLength = setup->wLength;

    memcpy(&last_setup_packet, setup, sizeof(last_setup_packet));

    int r = dev->controlTransfer(bmRequestType, bRequest, wValue, wIndex, buffer, wLength);
    return (r < 0) ? -1 : 0;
}
#endif
