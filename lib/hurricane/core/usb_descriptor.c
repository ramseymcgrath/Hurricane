#include "usb_descriptor.h"
#include <string.h>

int usb_parse_device_descriptor(const uint8_t* raw, usb_device_descriptor_t* desc)
{
    if (!raw || !desc) return -1;

    if (raw[0] != 18 || raw[1] != USB_DESC_TYPE_DEVICE) return -1;

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

int usb_parse_config_descriptor(const uint8_t* raw, usb_config_descriptor_t* desc)
{
    if (!raw || !desc) return -1;

    if (raw[0] < 9 || raw[1] != USB_DESC_TYPE_CONFIGURATION) return -1;

    desc->bLength = raw[0];
    desc->bDescriptorType = raw[1];
    desc->wTotalLength = (raw[3] << 8) | raw[2];
    desc->bNumInterfaces = raw[4];
    desc->bConfigurationValue = raw[5];
    desc->iConfiguration = raw[6];
    desc->bmAttributes = raw[7];
    desc->bMaxPower = raw[8];

    return 0;
}

int usb_parse_interface_descriptor(const uint8_t* raw, usb_interface_descriptor_t* desc)
{
    if (!raw || !desc) return -1;

    if (raw[0] < 9 || raw[1] != USB_DESC_TYPE_INTERFACE) return -1;

    desc->bLength = raw[0];
    desc->bDescriptorType = raw[1];
    desc->bInterfaceNumber = raw[2];
    desc->bAlternateSetting = raw[3];
    desc->bNumEndpoints = raw[4];
    desc->bInterfaceClass = raw[5];
    desc->bInterfaceSubClass = raw[6];
    desc->bInterfaceProtocol = raw[7];
    desc->iInterface = raw[8];

    return 0;
}

int usb_parse_endpoint_descriptor(const uint8_t* raw, usb_endpoint_descriptor_t* desc)
{
    if (!raw || !desc) return -1;

    if (raw[0] < 7 || raw[1] != USB_DESC_TYPE_ENDPOINT) return -1;

    desc->bLength = raw[0];
    desc->bDescriptorType = raw[1];
    desc->bEndpointAddress = raw[2];
    desc->bmAttributes = raw[3];
    desc->wMaxPacketSize = (raw[5] << 8) | raw[4];
    desc->bInterval = raw[6];

    return 0;
}

int usb_parse_hid_descriptor(const uint8_t* raw, usb_hid_descriptor_t* desc)
{
    if (!raw || !desc) return -1;

    if (raw[0] < 9 || raw[1] != USB_DESC_TYPE_HID) return -1;

    desc->bLength = raw[0];
    desc->bDescriptorType = raw[1];
    desc->bcdHID = (raw[3] << 8) | raw[2];
    desc->bCountryCode = raw[4];
    desc->bNumDescriptors = raw[5];
    desc->bDescriptorType2 = raw[6];
    desc->wDescriptorLength = (raw[8] << 8) | raw[7];

    return 0;
}
