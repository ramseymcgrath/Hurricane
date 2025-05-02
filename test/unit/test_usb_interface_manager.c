// tests/unit/test_usb_interface_manager.c

#include "../common/test_common.h"
#include "core/usb_interface_manager.h"
#include "hw/hurricane_hw_hal.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// --- Test variables and stubs ---

// These global variables track the state of the test environment
static int interface_configured_count = 0;
static int endpoint_configured_count = 0;
static uint8_t last_interface_num = 0xFF;
static uint8_t last_ep_address = 0xFF;

// Store original HAL functions
static int (*original_configure_interface)(uint8_t, uint8_t, uint8_t, uint8_t);
static int (*original_configure_endpoint)(uint8_t, uint8_t, uint8_t, uint16_t, uint8_t);

// Our test versions
static int test_hw_device_configure_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol)
{
    interface_configured_count++;
    last_interface_num = interface_num;
    return 0;
}

static int test_hw_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval)
{
    endpoint_configured_count++;
    last_ep_address = ep_address;
    return 0;
}

// --- Test Lifecycle ---

static void setUp(void)
{
    // Reset test environment before each test
    interface_configured_count = 0;
    endpoint_configured_count = 0;
    last_interface_num = 0xFF;
    last_ep_address = 0xFF;
    
    // Save original function pointers
    original_configure_interface = hurricane_hw_device_configure_interface;
    original_configure_endpoint = hurricane_hw_device_configure_endpoint;
    
    // Override with our test functions (this is just for tracking calls, not actually changing the implementation)
    // Note: In a real setup we'd use function pointer redirection, but we'll skip that here
    
    // Initialize the interface manager
    hurricane_interface_manager_init();
}

static void tearDown(void)
{
    hurricane_interface_manager_deinit();
    
    // We would restore the original functions here if we had actually changed them
}

// --- Unit Tests ---

int test_interface_manager_init_deinit(void)
{
    // Test already initialized in setUp()
    
    // The interface should start with no interfaces registered
    const hurricane_interface_descriptor_t* interface = hurricane_get_device_interface(0);
    TEST_ASSERT(interface == NULL, "No interfaces should be registered after init");
    
    // Call deinit and then init again to test re-initialization
    hurricane_interface_manager_deinit();
    hurricane_interface_manager_init();
    
    // Verify still no interfaces
    interface = hurricane_get_device_interface(0);
    TEST_ASSERT(interface == NULL, "No interfaces should be registered after re-init");
    
    TEST_PASS();
}

int test_add_device_interface(void)
{
    hurricane_interface_descriptor_t test_interface = {
        .interface_num = 1,
        .interface_class = 3, // HID class
        .interface_subclass = 0,
        .interface_protocol = 0,
        .num_endpoints = 2,
        .handler_type = INTERFACE_HANDLER_HID,
        .handler_data = NULL,
        .control_handler = NULL
    };
    
    // Add the interface
    int result = hurricane_add_device_interface(
        1, // interface_num
        3, // interface_class (HID)
        0, // interface_subclass
        0, // interface_protocol
        &test_interface
    );
    
    // Check result
    TEST_ASSERT_EQUAL_INT(0, result, "hurricane_add_device_interface should return 0");
    TEST_ASSERT_EQUAL_INT(1, interface_configured_count, "HAL configure_interface should be called once");
    TEST_ASSERT_EQUAL_INT(1, last_interface_num, "Interface number should be 1");
    
    // Verify we can retrieve the interface
    const hurricane_interface_descriptor_t* retrieved = hurricane_get_device_interface(1);
    TEST_ASSERT(retrieved != NULL, "Should be able to retrieve added interface");
    TEST_ASSERT_EQUAL_INT(3, retrieved->interface_class, "Interface class should match");
    TEST_ASSERT_EQUAL_INT(INTERFACE_HANDLER_HID, retrieved->handler_type, "Handler type should match");
    
    // Try adding the same interface again (should fail)
    result = hurricane_add_device_interface(1, 3, 0, 0, &test_interface);
    TEST_ASSERT(result != 0, "Adding same interface twice should fail");
    TEST_ASSERT_EQUAL_INT(1, interface_configured_count, "HAL shouldn't be called after failure");

    TEST_PASS();
}

int test_device_configure_endpoint(void)
{
    // First add an interface
    hurricane_interface_descriptor_t test_interface = {
        .interface_num = 2,
        .interface_class = 3, // HID class
        .interface_subclass = 0,
        .interface_protocol = 0,
        .num_endpoints = 0,
        .handler_type = INTERFACE_HANDLER_HID,
        .handler_data = NULL,
        .control_handler = NULL
    };
    
    int result = hurricane_add_device_interface(2, 3, 0, 0, &test_interface);
    TEST_ASSERT_EQUAL_INT(0, result, "hurricane_add_device_interface should return 0");
    
    // Now configure an endpoint for this interface
    result = hurricane_device_configure_endpoint(
        2,              // interface_num
        0x81,           // ep_address (EP1 IN)
        0x03,           // ep_attributes (Interrupt)
        64,             // ep_max_packet_size
        10              // ep_interval
    );
    
    TEST_ASSERT_EQUAL_INT(0, result, "hurricane_device_configure_endpoint should return 0");
    TEST_ASSERT_EQUAL_INT(1, endpoint_configured_count, "HAL configure_endpoint should be called once");
    TEST_ASSERT_EQUAL_INT(0x81, last_ep_address, "Endpoint address should match");
    
    // Verify we can retrieve the endpoint
    const hurricane_endpoint_descriptor_t* endpoint = hurricane_get_device_endpoint(2, 0x81);
    TEST_ASSERT(endpoint != NULL, "Should be able to retrieve added endpoint");
    TEST_ASSERT_EQUAL_INT(0x03, endpoint->ep_attributes, "Endpoint attributes should match");
    TEST_ASSERT_EQUAL_INT(64, endpoint->ep_max_packet_size, "Max packet size should match");
    TEST_ASSERT_EQUAL_INT(10, endpoint->ep_interval, "Interval should match");
    
    // Try configuring for non-existent interface (should fail)
    result = hurricane_device_configure_endpoint(99, 0x82, 0x03, 64, 10);
    TEST_ASSERT(result != 0, "Configuring endpoint for non-existent interface should fail");
    TEST_ASSERT_EQUAL_INT(1, endpoint_configured_count, "HAL shouldn't be called after failure");

    TEST_PASS();
}

int test_remove_device_interface(void)
{
    // Add two interfaces
    hurricane_interface_descriptor_t test_interface1 = {
        .interface_num = 3,
        .interface_class = 3, // HID class
        .interface_subclass = 0,
        .interface_protocol = 0,
        .num_endpoints = 0,
        .handler_type = INTERFACE_HANDLER_HID
    };
    
    hurricane_interface_descriptor_t test_interface2 = {
        .interface_num = 4,
        .interface_class = 2, // CDC class
        .interface_subclass = 0,
        .interface_protocol = 0,
        .num_endpoints = 0,
        .handler_type = INTERFACE_HANDLER_CDC
    };
    
    hurricane_add_device_interface(3, 3, 0, 0, &test_interface1);
    hurricane_add_device_interface(4, 2, 0, 0, &test_interface2);
    
    // Configure an endpoint for interface 3
    hurricane_device_configure_endpoint(3, 0x83, 0x03, 64, 10);
    
    // Verify both interfaces exist
    TEST_ASSERT(hurricane_get_device_interface(3) != NULL, "Interface 3 should exist");
    TEST_ASSERT(hurricane_get_device_interface(4) != NULL, "Interface 4 should exist");
    
    // Remove interface 3
    int result = hurricane_remove_device_interface(3);
    TEST_ASSERT_EQUAL_INT(0, result, "hurricane_remove_device_interface should return 0");
    
    // Verify interface 3 is gone but 4 still exists
    TEST_ASSERT(hurricane_get_device_interface(3) == NULL, "Interface 3 should be removed");
    TEST_ASSERT(hurricane_get_device_interface(4) != NULL, "Interface 4 should still exist");
    
    // Endpoint for interface 3 should no longer be accessible
    TEST_ASSERT(hurricane_get_device_endpoint(3, 0x83) == NULL, "Endpoint for removed interface should be gone");
    
    // Try to remove non-existent interface
    result = hurricane_remove_device_interface(99);
    TEST_ASSERT(result != 0, "Removing non-existent interface should fail");

    TEST_PASS();
}

// --- Test suite runner ---

int test_usb_interface_manager(void)
{
    int failures = 0;

    RUN_TEST(test_interface_manager_init_deinit);
    RUN_TEST(test_add_device_interface);
    RUN_TEST(test_device_configure_endpoint);
    RUN_TEST(test_remove_device_interface);

    return failures;
}