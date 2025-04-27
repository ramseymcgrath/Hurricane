#include "usb_hw_common.h"
#include <stdio.h>
#include <string.h>

// Dummy hardware state
static uint8_t dummy_device_descriptor[18] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB (2.00)
    0x00,       // bDeviceClass
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0
    0x6A, 0x0B, // idVendor (0x0B6A) - fake vendor
    0x46, 0x10, // idProduct (0x1046) - fake product
    0x00, 0x01, // bcdDevice (1.00)
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x03,       // iSerialNumber
    0x01        // bNumConfigurations
};

void usb_hw_reset_bus(void)
{
    printf("[hw] USB bus reset (simulated)\n");
}

void usb_hw_send_setup_packet(const usb_setup_packet_t* setup)
{
    printf("[hw] Sending SETUP packet:\n");
    printf("     bmRequestType=0x%02X, bRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X, wLength=%u\n",
           setup->bmRequestType, setup->bRequest, setup->wValue, setup->wIndex, setup->wLength);

    // TODO:
    // - Send the setup token
    // - Possibly wait for device ACK
    // - Prepare for data stage (IN or OUT)
    // - Then handle status stage
}

int usb_hw_receive_data(uint8_t* buffer, uint16_t length)
{
    printf("[hw] Receiving %u bytes from device (simulated)\n", length);

    if (length == 18) {
        memcpy(buffer, dummy_device_descriptor, 18);
        return 18; // Return number of bytes read
    }

    printf("[hw] Warning: simulated receive not implemented for length=%u\n", length);
    return 0;
}
