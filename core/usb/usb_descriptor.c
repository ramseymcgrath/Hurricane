#include "usb_descriptor.h"
#include "usb_control.h"
#include "usb_hw_hal.h"
#include <stdio.h>
#include <string.h>

int usb_parse_device_descriptor(const uint8_t* raw, usb_device_descriptor_t* desc)
{
    if (!raw || !desc) {
        return -1;
    }
    
    // Check valid descriptor length
    if (raw[0] != 18 || raw[1] != USB_DESC_TYPE_DEVICE) {
        return -1;
    }
    
    // Parse descriptor fields according to USB spec
    desc->bLength = raw[0];
    desc->bDescriptorType = raw[1];
    desc->bcdUSB = (raw[3] << 8) | raw[2];
    desc->bDeviceClass = raw[4];
    desc->bDeviceSubClass = raw[5];
    desc->bDeviceProtocol = raw[6];
    desc->bMaxPacketSize0 = raw[7];
    desc->idVendor = (raw[9] << 8) | raw[8];
    desc->idProduct = (raw[11] << 8) | raw[10];
    desc->bcdDevice = (raw[13] << 8) | raw[12];
    desc->iManufacturer = raw[14];
    desc->iProduct = raw[15];
    desc->iSerialNumber = raw[16];
    desc->bNumConfigurations = raw[17];
    
    return 0;
}

void usb_control_get_device_descriptor(uint8_t address)
{
    usb_setup_packet_t setup = {
        .bmRequestType = 0x80,
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_DEVICE << 8),
        .wIndex = 0,
        .wLength = 18,
    };

    usb_hw_send_setup(&setup);

    uint8_t buffer[18];
    int received = usb_hw_receive_control_data(buffer, sizeof(buffer));

    if (received == 18) {
        usb_device_descriptor_t desc;
        if (usb_parse_device_descriptor(buffer, &desc) == 0) {
            printf("[descriptor] Parsed Device Descriptor:\n");
            printf("  Vendor ID:  0x%04X\n", desc.idVendor);
            printf("  Product ID: 0x%04X\n", desc.idProduct);
            printf("  bcdUSB:     0x%04X\n", desc.bcdUSB);
            printf("  Class:      0x%02X\n", desc.bDeviceClass);
            printf("  MaxPacket0: %u\n", desc.bMaxPacketSize0);
            printf("  Configurations: %u\n", desc.bNumConfigurations);
        }
    } else {
        printf("[control] Failed to receive device descriptor\n");
    }
}
