// Dummy USB HAL for Hurricane project
// hw/boards/dummy/usb_hw_hal.c

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <usb/usb_control.h>

// Simulated Device Descriptor (USB spec 2.0 compliant, fake values)
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
    printf("[HAL-Dummy] Resetting USB bus...\n");
}

void usb_hw_enable_host_mode(void)
{
    printf("[HAL-Dummy] Enabling host mode\n");
}

void usb_hw_disable_host_mode(void)
{
    printf("[HAL-Dummy] Disabling host mode\n");
}

int usb_hw_send_setup(const usb_setup_packet_t* setup)
{
    printf("[HAL-Dummy] Sending SETUP packet:\n");
    printf("    bmRequestType=0x%02X, bRequest=0x%02X, wValue=0x%04X, wIndex=0x%04X, wLength=%u\n",
        setup->bmRequestType, setup->bRequest, setup->wValue, setup->wIndex, setup->wLength);
    return 0;
}

int usb_hw_receive_control_data(uint8_t* buffer, uint16_t length)
{
    printf("[HAL-Dummy] Receiving %u bytes from device (control IN)\n", length);

    if (length == 18) {
        memcpy(buffer, dummy_device_descriptor, 18);
        return 18;
    }

    memset(buffer, 0, length);
    return length;
}

int usb_hw_send_control_data(const uint8_t* buffer, uint16_t length)
{
    printf("[HAL-Dummy] Sending %u bytes to device (control OUT)\n", length);
    return length;
}

int usb_hw_control_in(uint8_t endpoint, uint8_t* buffer, uint16_t length)
{
    printf("[HAL-Dummy] IN transfer on endpoint %u, length %u\n", endpoint, length);
    memset(buffer, 0, length);
    return length;
}

int usb_hw_control_out(uint8_t endpoint, const uint8_t* buffer, uint16_t length)
{
    printf("[HAL-Dummy] OUT transfer on endpoint %u, length %u\n", endpoint, length);
    return length;
}
