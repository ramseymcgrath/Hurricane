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
