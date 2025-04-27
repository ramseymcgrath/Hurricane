#include "usb_control.h"
#include <stdio.h>

void usb_handle_setup_packet(const usb_setup_packet_t* setup)
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
    usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_SET_ADDRESS,
        .wValue = address,
        .wIndex = 0,
        .wLength = 0
    };
    if (usb_hw_send_setup(&setup) != 0) {
        printf("Failed to set USB device address to %u\n", address);
        return -1; // Indicate failure
    }
    printf("USB device address set to %u\n", address);
    return 0; // Indicate success
}

int usb_control_get_device_descriptor(uint8_t address)
{
    usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_DEVICE << 8),
        .wIndex = 0,
        .wLength = 18 // Standard device descriptor length
    };
    if (usb_hw_send_setup(&setup) != 0) {
        printf("Failed to request device descriptor for address %u\n", address);
        return -1; // Indicate failure
    }
    uint8_t buffer[18];
    if (usb_hw_receive_control_data(buffer, sizeof(buffer)) != 0) {
        printf("Failed to receive device descriptor for address %u\n", address);
        return -1; // Indicate failure
    }
    printf("Device descriptor received for address %u\n", address);
    // Further processing of the descriptor can be done here
    return 0; // Indicate success
}
