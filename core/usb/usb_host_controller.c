#include "usb_host_controller.h"
#include "usb_control.h"
#include "usb_hw_common.h"  // Uncommented this include
#include <stdio.h>

// USB standard request codes
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09

// USB descriptor types
#define USB_DESC_TYPE_DEVICE           0x01
#define USB_DESC_TYPE_CONFIGURATION    0x02
#define USB_DESC_TYPE_STRING           0x03
#define USB_DESC_TYPE_INTERFACE        0x04
#define USB_DESC_TYPE_ENDPOINT         0x05
#define USB_DESC_TYPE_HID              0x21
#define USB_DESC_TYPE_REPORT           0x22
#define USB_DESC_TYPE_PHYSICAL         0x23

static usb_device_t device;

void usb_host_init(void)
{
    device.state = DEVICE_STATE_DEFAULT;
    device.device_address = 0;
    
    usb_hw_reset_bus(); // Reset the USB bus
    printf("[host] Bus reset initiated\n");
}

void usb_host_poll(void)
{
    switch (device.state)
    {
        case DEVICE_STATE_DEFAULT:
            printf("[host] Setting device address...\n");
            usb_control_set_address(1); // Assign address 1
            device.device_address = 1;
            device.state = DEVICE_STATE_ADDRESS;
            break;

        case DEVICE_STATE_ADDRESS:
            printf("[host] Fetching device descriptor...\n");
            usb_control_get_device_descriptor(device.device_address);
            device.state = DEVICE_STATE_CONFIGURED;
            break;

        case DEVICE_STATE_CONFIGURED:
            printf("[host] Device enumeration complete.\n");
            // Ready to talk HID
            break;

        case DEVICE_STATE_ERROR:
        default:
            printf("[host] Device in error state.\n");
            break;
    }
}

void usb_control_set_address(uint8_t address)
{
    usb_setup_packet_t setup = {
        .bmRequestType = 0x00, // Host to device, standard, device
        .bRequest = USB_REQ_SET_ADDRESS,
        .wValue = address,
        .wIndex = 0,
        .wLength = 0,
    };

    usb_hw_send_setup_packet(&setup);
    printf("[control] Sent SET_ADDRESS %u\n", address);
}

void usb_control_get_device_descriptor(uint8_t address)
{
    usb_setup_packet_t setup = {
        .bmRequestType = 0x80, // Device to host, standard, device
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_DEVICE << 8), // Descriptor Type in high byte
        .wIndex = 0,
        .wLength = 18, // Device descriptor is 18 bytes
    };

    usb_hw_send_setup_packet(&setup);
    printf("[control] Requested DEVICE descriptor\n");

    // TODO: receive descriptor buffer
}

