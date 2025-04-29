// tests/unit/test_usb_control.c

#include "../common/test_common.h"
#include "hurricane/usb/usb_control.h"
#include "hurricane/hw/usb_hw_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// --- Stubs and Mocking ---

usb_hw_setup_packet_t last_setup_sent;
uint8_t last_control_data_sent[64];
size_t last_control_data_length = 0;

int usb_hw_send_control_transfer(const usb_hw_setup_packet_t* setup, void* buffer, uint16_t length)
{
    memcpy(&last_setup_sent, setup, sizeof(usb_hw_setup_packet_t));
    if (buffer && length > 0) {
        memcpy(last_control_data_sent, buffer, length);
        last_control_data_length = length;
    } else {
        last_control_data_length = 0;
    }
    return 0; // Pretend success
}

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
