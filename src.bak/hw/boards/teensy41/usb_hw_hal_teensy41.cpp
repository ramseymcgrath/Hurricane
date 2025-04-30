// src/hw/boards/teensy41/usb_hw_hal_teensy41.c

#include "hw/usb_hw_hal.h"
#if defined(PLATFORM_TEENSY41)
#include <Arduino.h>
#include <USBHost_t36.h>

static USBHost myusb;
static USBDevice* dev = nullptr;
static USBHub hub1(myusb);
static USBHub hub2(myusb);
static USBHub hub3(myusb);

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
    USBDevice* current = myusb.getDevice();
    if (current && current->address > 0) {
        dev = current;
        return 1;
    }
    dev = nullptr;
    return 0;
}

int usb_hw_reset_bus(void)
{
    dev = nullptr;
    myusb.end();
    delay(100);
    myusb.begin();
    return 0;
}

int usb_hw_send_control_transfer(const usb_hw_setup_packet_t* setup, void* buffer, uint16_t length)
{
    if (!dev) {
        return -1;
    }

    memcpy(&last_setup_packet, setup, sizeof(last_setup_packet));

    int result = dev->controlTransfer(
        setup->bmRequestType,
        setup->bRequest,
        setup->wValue,
        setup->wIndex,
        buffer,
        setup->wLength
    );

    return (result < 0) ? -1 : 0;
}

#endif
