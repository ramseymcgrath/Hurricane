#include "core/usb_host_controller.h"
#include "hw/usb_hw_hal.h"
#include "core/usb_descriptor.h"
#include "usb_control.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void usb_handle_setup_packet(const usb_hw_setup_packet_t* setup)
{
    printf("USB SETUP packet:\n");
    printf("  bmRequestType: 0x%02X\n", setup->bmRequestType);
    printf("  bRequest:      0x%02X\n", setup->bRequest);
    printf("  wValue:        0x%04X\n", setup->wValue);
    printf("  wIndex:        0x%04X\n", setup->wIndex);
    printf("  wLength:       %u\n", setup->wLength);
}

int usb_control_set_address(uint8_t address)
{
    usb_hw_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_SET_ADDRESS,
        .wValue = address,
        .wIndex = 0,
        .wLength = 0
    };

    if (usb_hw_send_control_transfer(&setup, NULL, 0) != 0) {
        printf("[usb_control] Failed to set USB device address to %u\n", address);
        return -1;
    }

    printf("[usb_control] USB device address set to %u\n", address);
    return 0;
}

int usb_control_get_device_descriptor(uint8_t address, usb_device_descriptor_t* desc_out)
{
    if (!desc_out) {
        return -1;
    }

    usb_hw_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_DEVICE << 8),
        .wIndex = 0,
        .wLength = USB_DEVICE_DESCRIPTOR_SIZE
    };

    uint8_t buffer[USB_DEVICE_DESCRIPTOR_SIZE];

    if (usb_hw_send_control_transfer(&setup, buffer, sizeof(buffer)) != 0) {
        printf("[usb_control] Failed to request device descriptor for address %u\n", address);
        return -1;
    }

    if (usb_parse_device_descriptor(buffer, desc_out) != 0) {
        printf("[usb_control] Failed to parse device descriptor\n");
        return -1;
    }

    printf("[usb_control] Successfully parsed device descriptor for address %u\n", address);
    return 0;
}
