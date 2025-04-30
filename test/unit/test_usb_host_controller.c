// tests/unit/test_usb_host_controller.c

#include "../common/test_common.h"
#include "core/usb_host_controller.h"
#include "usb/usb_control.h"
#include "hw/hurricane_hw_hal.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// --- Stubs for dependencies ---

// These variables are defined in the dummy HAL implementation
extern uint8_t test_address_set;
extern uint8_t test_descriptor_requested;

// Make these variables extern so they can be modified by the dummy HAL
extern hurricane_usb_setup_packet_t last_setup_sent;

void test_usb_hw_reset_bus(void)
{
    printf("[test stub] usb_hw_reset_bus called\n");
}

// --- Test Lifecycle ---

void setUp(void)
{
    test_address_set = 0;
    test_descriptor_requested = 0;
}

void tearDown(void)
{
    // nothing needed
}

// --- Unit Tests ---

int test_usb_host_poll_sequence(void)
{
    usb_host_init();

    // DEVICE_STATE_DEFAULT -> sets address
    usb_host_poll();
    TEST_ASSERT_EQUAL_INT(1, test_address_set, "Expected address to be set to 1 in DEVICE_STATE_DEFAULT");

    // DEVICE_STATE_ADDRESS -> gets descriptor
    test_address_set = 0;
    usb_host_poll();
    TEST_ASSERT_EQUAL_INT(1, test_descriptor_requested, "Expected descriptor request for address 1");

    // DEVICE_STATE_CONFIGURED -> should do nothing
    test_descriptor_requested = 0;
    usb_host_poll();
    TEST_ASSERT_EQUAL_INT(0, test_address_set, "No new address setting expected");
    TEST_ASSERT_EQUAL_INT(0, test_descriptor_requested, "No new descriptor request expected");

    TEST_PASS();
}

int test_usb_host_controller(void)
{
    int failures = 0;

    RUN_TEST(test_usb_host_poll_sequence);

    return failures;
}
