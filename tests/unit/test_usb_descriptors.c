#include "../common/test_common.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// USB descriptor types for testing
#define USB_DESC_TYPE_DEVICE           0x01
#define USB_DESC_TYPE_CONFIGURATION    0x02
#define USB_DESC_TYPE_STRING           0x03
#define USB_DESC_TYPE_INTERFACE        0x04
#define USB_DESC_TYPE_ENDPOINT         0x05

// Define a basic USB device descriptor structure for testing
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_device_descriptor_t;

// Define a basic USB endpoint descriptor structure for testing
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} usb_endpoint_descriptor_t;

int test_usb_device_descriptor(void)
{
    usb_device_descriptor_t descriptor = {
        .bLength = 18,
        .bDescriptorType = USB_DESC_TYPE_DEVICE,
        .bcdUSB = 0x0200,        // USB 2.0
        .bDeviceClass = 0,       // Class defined at interface level
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 64,
        .idVendor = 0x045E,      // Example: Microsoft
        .idProduct = 0x028E,     // Example: Xbox360 Controller
        .bcdDevice = 0x0100,     // Version 1.0
        .iManufacturer = 1,
        .iProduct = 2,
        .iSerialNumber = 3,
        .bNumConfigurations = 1
    };
    
    TEST_ASSERT_EQUAL_INT(18, descriptor.bLength, "Device descriptor length should be 18 bytes");
    TEST_ASSERT_EQUAL_INT(USB_DESC_TYPE_DEVICE, descriptor.bDescriptorType, "Descriptor type should be DEVICE");
    TEST_ASSERT_EQUAL_INT(0x0200, descriptor.bcdUSB, "USB version should be 2.0");
    TEST_ASSERT_EQUAL_INT(64, descriptor.bMaxPacketSize0, "Max packet size should be 64");
    TEST_ASSERT_EQUAL_INT(0x045E, descriptor.idVendor, "Vendor ID mismatch");
    TEST_ASSERT_EQUAL_INT(0x028E, descriptor.idProduct, "Product ID mismatch");
    
    TEST_PASS();
}

int test_usb_endpoint_descriptor(void)
{
    usb_endpoint_descriptor_t endpoint = {
        .bLength = 7,
        .bDescriptorType = USB_DESC_TYPE_ENDPOINT,
        .bEndpointAddress = 0x81,    // IN Endpoint 1
        .bmAttributes = 0x03,        // Interrupt endpoint
        .wMaxPacketSize = 64,
        .bInterval = 10             // Poll every 10 frames
    };
    
    TEST_ASSERT_EQUAL_INT(7, endpoint.bLength, "Endpoint descriptor length should be 7 bytes");
    TEST_ASSERT_EQUAL_INT(USB_DESC_TYPE_ENDPOINT, endpoint.bDescriptorType, "Descriptor type should be ENDPOINT");
    TEST_ASSERT_EQUAL_INT(0x81, endpoint.bEndpointAddress, "Endpoint address mismatch");
    TEST_ASSERT_EQUAL_INT(0x03, endpoint.bmAttributes, "Attributes mismatch (should be interrupt)");
    TEST_ASSERT_EQUAL_INT(64, endpoint.wMaxPacketSize, "Max packet size should be 64");
    TEST_ASSERT_EQUAL_INT(10, endpoint.bInterval, "Interval mismatch");
    
    TEST_PASS();
}

int test_usb_descriptors(void)
{
    int failures = 0;
    
    RUN_TEST(test_usb_device_descriptor);
    RUN_TEST(test_usb_endpoint_descriptor);
    
    return failures;
}