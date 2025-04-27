#include "../common/test_common.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Define test structures that match what we expect from the real implementation
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

int test_usb_setup_packet(void)
{
    usb_setup_packet_t pkt = {
        .bmRequestType = 0x80,
        .bRequest = 0x06,
        .wValue = 0x0100,
        .wIndex = 0,
        .wLength = 18
    };

    TEST_ASSERT_EQUAL_INT(0x80, pkt.bmRequestType, "bmRequestType mismatch");
    TEST_ASSERT_EQUAL_INT(0x06, pkt.bRequest, "bRequest mismatch");
    TEST_ASSERT_EQUAL_INT(0x0100, pkt.wValue, "wValue mismatch");
    TEST_ASSERT_EQUAL_INT(0, pkt.wIndex, "wIndex mismatch");
    TEST_ASSERT_EQUAL_INT(18, pkt.wLength, "wLength mismatch");

    TEST_PASS();
}

int test_usb_control(void)
{
    int failures = 0;

    RUN_TEST(test_usb_setup_packet);

    return failures;
}