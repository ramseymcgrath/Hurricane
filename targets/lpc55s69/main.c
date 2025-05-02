/**
 * @file main.c
 * @brief Main entry point for the LPC55S69 dual USB stack example
 * 
 * This example demonstrates using both USB host and device stacks simultaneously
 * on the LPC55S69 platform with dynamic interface configuration.
 */

#include "hurricane.h"
#include "hurricane_hw_hal.h"
#include "core/usb_interface_manager.h"
#include <stdio.h>
#include <string.h>

// HID report descriptor for a simple mouse
static const uint8_t mouse_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (Button 1)
    0x29, 0x03,        //     Usage Maximum (Button 3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data, Variable, Absolute)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x03,        //     Input (Constant)
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    0xC0,              //   End Collection
    0xC0               // End Collection
};

// Mouse report structure
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
} mouse_report_t;

// Device descriptors
static uint8_t device_descriptor[] = {
    18,                           // Size of this descriptor in bytes
    1,                            // DEVICE descriptor type
    0x00, 0x02,                   // USB Specification Release Number (2.0)
    0,                            // Device class
    0,                            // Device subclass
    0,                            // Device protocol
    64,                           // Maximum packet size for endpoint zero
    0x55, 0xAA,                   // Vendor ID (dummy)
    0x30, 0x31,                   // Product ID (dummy)
    0x00, 0x01,                   // Device release number (1.0)
    1,                            // Index of manufacturer string descriptor
    2,                            // Index of product string descriptor
    0,                            // Index of serial number string descriptor
    1                             // Number of possible configurations
};

static uint8_t config_descriptor[] = {
    // Configuration descriptor
    9,                            // Size of this descriptor in bytes
    2,                            // CONFIGURATION descriptor type
    34, 0,                        // Total length
    1,                            // Number of interfaces
    1,                            // Configuration value
    0,                            // Index of string descriptor
    0x80,                         // Configuration attributes (bus powered)
    50,                           // Maximum power (100mA)

    // Interface descriptor
    9,                            // Size of this descriptor in bytes
    4,                            // INTERFACE descriptor type
    0,                            // Interface number
    0,                            // Alternate setting
    1,                            // Number of endpoints
    3,                            // Interface class (HID)
    0,                            // Interface subclass
    2,                            // Interface protocol (mouse)
    0,                            // Index of string descriptor

    // HID descriptor
    9,                            // Size of this descriptor in bytes
    0x21,                         // HID descriptor type
    0x11, 0x01,                   // HID specification version (1.11)
    0,                            // Country code
    1,                            // Number of class descriptors
    0x22,                         // Report descriptor type
    sizeof(mouse_report_descriptor), 0, // Report descriptor length

    // Endpoint descriptor
    7,                            // Size of this descriptor in bytes
    5,                            // ENDPOINT descriptor type
    0x81,                         // Endpoint address (IN endpoint 1)
    0x03,                         // Attributes (interrupt)
    4, 0,                         // Maximum packet size (4 bytes)
    10                            // Polling interval (10ms)
};

// Forward declarations
static bool hid_control_request_handler(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);
static void hid_send_report_callback(uint8_t* buffer, uint16_t length);
static void hid_receive_report_callback(uint8_t* buffer, uint16_t length);
static bool hid_match_device(uint8_t device_class, uint8_t device_subclass, uint8_t device_protocol);
static void hid_attach_device(void* device);
static void hid_detach_device(void* device);
static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);
static void hid_data_callback(uint8_t endpoint, void* buffer, uint16_t length);
static void configure_device_mode(void);
static void configure_host_mode(void);
static void generate_mouse_movement(void);

int main(void) {
    // Initialize hardware
    hurricane_hw_init();
    printf("🚀 Hurricane dual USB stack booted on LPC55S69\n");
    
    // Initialize the interface manager
    hurricane_interface_manager_init();
    printf("Interface manager initialized\n");
    
    // Configure device mode (USB1 controller)
    configure_device_mode();
    
    // Configure host mode (USB0 controller)
    configure_host_mode();
    
    // Ensure proper synchronization between controllers
    hurricane_hw_sync_controllers();
    printf("USB controllers synchronized for dual operation\n");
    
    uint32_t mouse_timer = 0;
    
    // Main loop
    while (1) {
        // Run the USB stack polling functions
        hurricane_task();  // This runs both host and device stacks
        
        // Generate simulated mouse movement every 100ms
        if (mouse_timer++ >= 100) {
            mouse_timer = 0;
            generate_mouse_movement();
        }
        
        // Simple delay - LPC specific delay function could be used here
        for (volatile int i = 0; i < 1000; i++) {
            __asm("nop");
        }
    }
    
    return 0;  // Never reached
}

/**
 * Configure the USB device mode with a HID mouse interface
 */
static void configure_device_mode(void) {
    // Create descriptors structure
    hurricane_device_descriptors_t descriptors = {0};
    descriptors.device_descriptor = device_descriptor;
    descriptors.device_descriptor_length = sizeof(device_descriptor);
    descriptors.config_descriptor = config_descriptor;
    descriptors.config_descriptor_length = sizeof(config_descriptor);
    descriptors.hid_report_descriptor = (uint8_t*)mouse_report_descriptor;
    descriptors.hid_report_descriptor_length = sizeof(mouse_report_descriptor);
    
    // Update device descriptors
    hurricane_device_update_descriptors(&descriptors);
    
    // Define the HID interface
    hurricane_interface_descriptor_t hid_interface = {
        .interface_num = 0,
        .interface_class = 3,  // HID class
        .interface_subclass = 0,
        .interface_protocol = 2,  // Mouse
        .num_endpoints = 1,
        .handler_type = INTERFACE_HANDLER_HID,
        .handler_data = NULL,
        .control_handler = hid_control_request_handler
    };
    
    // Add the HID interface
    hurricane_add_device_interface(0, 3, 0, 2, &hid_interface);
    
    // Configure the interrupt IN endpoint
    hurricane_device_configure_endpoint(
        0,        // interface number
        0x81,     // endpoint address (IN, EP1)
        0x03,     // interrupt endpoint
        4,        // max packet size
        10        // polling interval (10ms)
    );
    
    // Register HID report callbacks
    hurricane_device_hid_register_callbacks(
        hid_send_report_callback,
        hid_receive_report_callback
    );
    
    printf("USB device mode configured as HID mouse\n");
}

/**
 * Configure the USB host mode to detect HID devices
 */
static void configure_host_mode(void) {
    // Create HID host handler
    hurricane_host_class_handler_t hid_handler = {
        .match_callback = hid_match_device,
        .attach_callback = hid_attach_device,
        .detach_callback = hid_detach_device,
        .control_callback = hid_control_callback,
        .data_callback = hid_data_callback
    };
    
    // Register the HID handler for the host stack
    hurricane_register_host_class_handler(
        3,   // HID class
        0,   // Any subclass
        0,   // Any protocol
        &hid_handler
    );
    
    printf("USB host mode configured to detect HID devices\n");
}

/**
 * Generate simulated mouse movement when device is connected to a host
 */
static void generate_mouse_movement(void) {
    static uint32_t count = 0;
    static int8_t directions[] = {5, 0, -5, 0};
    
    // Only send reports if a host is connected to our device
    if (hurricane_hw_device_host_connected()) {
        mouse_report_t report = {0};
        
        // Create a circular motion
        report.buttons = 0;
        report.x = directions[count % 4];
        report.y = directions[(count + 1) % 4];
        count++;
        
        // Send the report
        hurricane_device_hid_send_report((uint8_t*)&report, sizeof(report));
    }
}

// HID device callbacks
static bool hid_control_request_handler(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length) {
    // Handle HID-specific control requests here
    printf("HID control request: 0x%02x\n", setup->bRequest);
    return true;
}

static void hid_send_report_callback(uint8_t* buffer, uint16_t length) {
    printf("HID report sent: %d bytes\n", length);
}

static void hid_receive_report_callback(uint8_t* buffer, uint16_t length) {
    printf("HID report received: %d bytes\n", length);
}

// HID host callbacks
static bool hid_match_device(uint8_t device_class, uint8_t device_subclass, uint8_t device_protocol) {
    return (device_class == 3);  // HID class
}

static void hid_attach_device(void* device) {
    printf("HID device attached to host controller\n");
    
    // TODO: Handle device enumeration and configuration here
}

static void hid_detach_device(void* device) {
    printf("HID device detached from host controller\n");
}

static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length) {
    printf("Host control request to HID device\n");
    return true;
}

static void hid_data_callback(uint8_t endpoint, void* buffer, uint16_t length) {
    printf("Host received data from HID device: %d bytes\n", length);
    
    // Process the received data if it's a mouse report
    if (length >= 3) {
        mouse_report_t* report = (mouse_report_t*)buffer;
        printf("  Mouse movement: buttons=0x%02x, x=%d, y=%d\n", 
               report->buttons, report->x, report->y);
    }
}