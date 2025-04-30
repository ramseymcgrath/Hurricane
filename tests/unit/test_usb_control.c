// tests/unit/test_usb_control.c

#include "../common/test_common.h"
#include "usb/usb_control.h"
#include "hw/hurricane_hw_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// --- Stubs and Mocking ---

// These globals are defined in the HAL implementation, we just reference them
extern hurricane_usb_setup_packet_t last_setup_sent;
extern uint8_t last_control_data_sent[64];
extern size_t last_control_data_length;

// --- Unit Tests ---

int test_usb_control_set_address(void)
{
    memset(&last_setup_sent, 0, sizeof(last_setup_sent));

    int result = usb_control_set_address(5);

    TEST_ASSERT_EQUAL_INT(0, result, "usb_control_set_address should return 0");
    TEST_ASSERT_EQUAL_INT(0x00, last_setup_sent.bmRequestType, "bmRequestType should be 0x00 (host to device)");
    TEST_ASSERT_EQUAL_INT(USB_REQ_SET_ADDRESS, last_setup_sent.bRequest, "bRequest should be SET_ADDRESS");
    TEST_ASSERT_EQUAL_INT(5, last_setup_sent.wValue, "wValue should match address 5");
    TEST_ASSERT_EQUAL_INT(0, last_setup_sent.wIndex, "wIndex should be 0");
    TEST_ASSERT_EQUAL_INT(0, last_setup_sent.wLength, "wLength should be 0");

    TEST_PASS();
}

int test_usb_control_get_device_descriptor(void)
{
    memset(&last_setup_sent, 0, sizeof(last_setup_sent));

    usb_device_descriptor_t dummy_desc;
    int result = usb_control_get_device_descriptor(1, &dummy_desc);

    TEST_ASSERT_EQUAL_INT(0, result, "usb_control_get_device_descriptor should return 0");
    TEST_ASSERT_EQUAL_INT(0x80, last_setup_sent.bmRequestType, "bmRequestType should be 0x80 (device to host)");
    TEST_ASSERT_EQUAL_INT(USB_REQ_GET_DESCRIPTOR, last_setup_sent.bRequest, "bRequest should be GET_DESCRIPTOR");
    TEST_ASSERT_EQUAL_INT((USB_DESC_TYPE_DEVICE << 8), last_setup_sent.wValue, "wValue should request DEVICE descriptor");
    TEST_ASSERT_EQUAL_INT(0, last_setup_sent.wIndex, "wIndex should be 0");

    TEST_PASS();
}

// --- Test suite runner ---

int test_usb_control(void)
{
    int failures = 0;

    RUN_TEST(test_usb_control_set_address);
    RUN_TEST(test_usb_control_get_device_descriptor);

    return failures;
}
