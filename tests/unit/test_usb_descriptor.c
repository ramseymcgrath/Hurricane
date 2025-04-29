#include "../common/test_common.h"
#include "usb/usb_control.h"
#include "hw/usb_hw_hal.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

int test_usb_parse_device_descriptor(void)
{
    uint8_t raw_descriptor[18] = {
        0x12,       // bLength
        0x01,       // bDescriptorType (Device)
        0x00, 0x02, // bcdUSB (2.00)
        0x00,       // bDeviceClass
        0x00,       // bDeviceSubClass
        0x00,       // bDeviceProtocol
        0x40,       // bMaxPacketSize0
        0x5E, 0x04, // idVendor (0x045E) - Microsoft
        0x8E, 0x02, // idProduct (0x028E) - Xbox360 Controller
        0x00, 0x01, // bcdDevice (1.00)
        0x01,       // iManufacturer
        0x02,       // iProduct
        0x03,       // iSerialNumber
        0x01        // bNumConfigurations
    };

    usb_device_descriptor_t parsed;
    memset(&parsed, 0, sizeof(parsed));

    int result = usb_parse_device_descriptor(raw_descriptor, &parsed);
    
    TEST_ASSERT_EQUAL_INT(0, result, "usb_parse_device_descriptor() should succeed");

    // Validate parsed fields
    TEST_ASSERT_EQUAL_INT(0x0200, parsed.bcdUSB, "Parsed bcdUSB should be 0x0200");
    TEST_ASSERT_EQUAL_INT(0x045E, parsed.idVendor, "Parsed idVendor should be 0x045E");
    TEST_ASSERT_EQUAL_INT(0x028E, parsed.idProduct, "Parsed idProduct should be 0x028E");
    TEST_ASSERT_EQUAL_INT(0x0100, parsed.bcdDevice, "Parsed bcdDevice should be 0x0100");
    TEST_ASSERT_EQUAL_INT(0x40, parsed.bMaxPacketSize0, "Parsed bMaxPacketSize0 should be 64");
    TEST_ASSERT_EQUAL_INT(0x01, parsed.bNumConfigurations, "Parsed bNumConfigurations should be 1");

    TEST_PASS();
}

int test_usb_descriptor(void)
{
    int failures = 0;
    RUN_TEST(test_usb_parse_device_descriptor);
    return failures;
}
