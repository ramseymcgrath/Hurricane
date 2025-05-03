/**
 * @file usb_interface_manager_demo.c
 * @brief Demonstration of the USB Interface Manager for dual HID interfaces on LPC55S69
 * 
 * This file demonstrates how to use the Hurricane USB Interface Manager
 * to dynamically configure multiple USB interfaces, including adding and
 * configuring interfaces, endpoints, and setting up control handlers.
 * 
 * For the LPC55S69, it utilizes:
 * - USB0 in device mode (Full-Speed PHY)
 * - USB1 in host mode (High-Speed EHCI)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hurricane.h"
#include "core/usb_interface_manager.h"

// Sample HID report descriptor for a mouse
static const uint8_t hid_mouse_report_descriptor[] = {
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
    0x81, 0x03,        //     Input (Constant) - padding
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

// Sample HID report descriptor for a keyboard
static const uint8_t hid_keyboard_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xE7,        //   Usage Maximum (Keyboard Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute) - Modifier byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Constant) - Reserved byte
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x19, 0x00,        //   Usage Minimum (Reserved)
    0x29, 0x65,        //   Usage Maximum (Keyboard Application)
    0x81, 0x00,        //   Input (Data, Array) - Key arrays (6 bytes)
    0xC0               // End Collection
};

// Sample device descriptor
static uint8_t device_descriptor[] = {
    18,                // bLength
    1,                 // bDescriptorType (Device)
    0x00, 0x02,        // bcdUSB 2.0
    0,                 // bDeviceClass (Use class info from Interface Descriptors)
    0,                 // bDeviceSubClass
    0,                 // bDeviceProtocol
    64,                // bMaxPacketSize0
    0xC0, 0x16,        // idVendor
    0x5C, 0x55,        // idProduct (LPC55S69 Dual HID)
    0x01, 0x01,        // bcdDevice
    1,                 // iManufacturer
    2,                 // iProduct
    3,                 // iSerialNumber
    1                  // bNumConfigurations
};

// Placeholder for the configuration descriptor
// The actual configuration descriptor will be built dynamically
static uint8_t config_descriptor_template[] = {
    9,                 // bLength
    2,                 // bDescriptorType (Configuration)
    0, 0,              // wTotalLength - will be filled in
    0,                 // bNumInterfaces - will be filled in
    1,                 // bConfigurationValue
    0,                 // iConfiguration (String Index)
    0x80,              // bmAttributes (Bus Powered)
    50,                // bMaxPower (100mA)
    
    // Interface descriptors will be added dynamically
};

// HID configuration data
typedef struct {
    uint8_t interface_num;
    const uint8_t* report_descriptor;
    uint16_t report_descriptor_length;
    uint8_t in_endpoint;
    bool configured;
} hid_config_t;

static hid_config_t hid_configs[] = {
    { 0, hid_mouse_report_descriptor, sizeof(hid_mouse_report_descriptor), 0x81, false },
    { 1, hid_keyboard_report_descriptor, sizeof(hid_keyboard_report_descriptor), 0x82, false }
};

// Callbacks
static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);
static void configuration_callback(uint8_t configuration);
static void interface_callback(uint8_t interface, uint8_t alt_setting);

// Function to dynamically build a complete configuration descriptor with interfaces
static uint8_t* build_configuration_descriptor(size_t* total_length);

// Function to add a HID interface at runtime
static int add_hid_interface(uint8_t interface_num, const uint8_t* report_descriptor, uint16_t descriptor_length);

/**
 * Initialize the dual HID demonstration
 */
void usb_interface_manager_demo_init(void)
{
    printf("Initializing USB Interface Manager Demo for LPC55S69\n");
    
    // Initialize the USB Interface Manager
    hurricane_interface_manager_init();
    
    // Register configuration callback
    hurricane_hw_device_set_configuration_callback(configuration_callback);
    
    // Register interface callback
    hurricane_hw_device_set_interface_callback(interface_callback);
    
    // Build the initial configuration descriptor
    size_t config_desc_len;
    uint8_t* config_desc = build_configuration_descriptor(&config_desc_len);
    
    if (!config_desc) {
        printf("Failed to build configuration descriptor!\n");
        return;
    }
    
    // Set up the device descriptors
    hurricane_device_descriptors_t descriptors = {0};
    descriptors.device_descriptor = device_descriptor;
    descriptors.device_descriptor_length = sizeof(device_descriptor);
    descriptors.config_descriptor = config_desc;
    descriptors.config_descriptor_length = config_desc_len;
    
    // Update device descriptors
    hurricane_device_update_descriptors(&descriptors);
    
    // Free the dynamically allocated descriptor
    free(config_desc);
    
    printf("USB Interface Manager Demo initialized for LPC55S69\n");
    printf("- USB0: Device Mode (Full Speed PHY)\n");
    printf("- USB1: Host Mode (High Speed EHCI)\n");
}

/**
 * Run the dual HID demonstration - called periodically from main loop
 */
void usb_interface_manager_demo_task(void)
{
    static bool interfaces_configured = false;
    
    // If we receive a configuration from the host, configure our interfaces
    if (!interfaces_configured && hurricane_hw_device_host_connected()) {
        printf("Host connected to USB0, configuring interfaces\n");
        
        // Add Mouse HID interface
        add_hid_interface(
            hid_configs[0].interface_num,
            hid_configs[0].report_descriptor,
            hid_configs[0].report_descriptor_length
        );
        
        // Add Keyboard HID interface
        add_hid_interface(
            hid_configs[1].interface_num,
            hid_configs[1].report_descriptor,
            hid_configs[1].report_descriptor_length
        );
        
        interfaces_configured = true;
        
        // Trigger device reset to apply new configuration
        hurricane_device_trigger_reset();
        
        printf("Interfaces configured on USB0\n");
    }
    
    // Example: send mouse movement if connected
    if (interfaces_configured && hid_configs[0].configured) {
        static uint32_t last_mouse_time = 0;
        uint32_t current_time = hurricane_get_time_ms();
        
        if (current_time - last_mouse_time > 1000) {
            // Simple mouse movement: move right and down
            uint8_t mouse_report[] = { 0x00, 0x05, 0x05 };
            
            hurricane_hw_device_interrupt_in_transfer(
                hid_configs[0].in_endpoint,
                mouse_report,
                sizeof(mouse_report)
            );
            
            last_mouse_time = current_time;
        }
    }
    
    // Example: send keyboard press if connected
    if (interfaces_configured && hid_configs[1].configured) {
        static uint32_t last_keyboard_time = 0;
        uint32_t current_time = hurricane_get_time_ms();
        
        if (current_time - last_keyboard_time > 2000) {
            // Press and release the 'H' key
            uint8_t keyboard_report[] = { 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00 };
            
            hurricane_hw_device_interrupt_in_transfer(
                hid_configs[1].in_endpoint,
                keyboard_report,
                sizeof(keyboard_report)
            );
            
            // Release all keys after a short delay
            hurricane_delay_ms(50);
            memset(keyboard_report + 2, 0, 6);
            
            hurricane_hw_device_interrupt_in_transfer(
                hid_configs[1].in_endpoint,
                keyboard_report,
                sizeof(keyboard_report)
            );
            
            last_keyboard_time = current_time;
        }
    }
}

/**
 * Clean up the USB Interface Manager Demo resources
 */
void usb_interface_manager_demo_deinit(void)
{
    // Deinitialize the Interface Manager
    hurricane_interface_manager_deinit();
    
    printf("USB Interface Manager Demo deinitialized\n");
}

/**
 * Add a HID interface at runtime
 */
static int add_hid_interface(uint8_t interface_num, const uint8_t* report_descriptor, uint16_t descriptor_length)
{
    printf("Adding HID interface %d\n", interface_num);
    
    // Create interface descriptor
    hurricane_interface_descriptor_t interface_desc = {
        .interface_num = interface_num,
        .interface_class = 0x03,         // HID Class
        .interface_subclass = 0x01,      // Boot interface subclass
        .interface_protocol = (interface_num == 0) ? 0x02 : 0x01, // Mouse or Keyboard
        .num_endpoints = 1,
        .handler_type = INTERFACE_HANDLER_HID,
        .handler_data = NULL,
        .control_handler = hid_control_callback
    };
    
    // Register the interface
    int result = hurricane_add_device_interface(
        interface_num,
        interface_desc.interface_class,
        interface_desc.interface_subclass,
        interface_desc.interface_protocol,
        &interface_desc
    );
    
    if (result != 0) {
        printf("Failed to add interface %d, error %d\n", interface_num, result);
        return result;
    }
    
    // Configure the IN endpoint
    uint8_t ep_address = 0x80 | (interface_num + 1); // IN endpoint, unique per interface
    result = hurricane_device_configure_endpoint(
        interface_num,
        ep_address,
        0x03,                // Interrupt endpoint
        64,                  // Max packet size
        10                   // 10ms polling interval
    );
    
    if (result != 0) {
        printf("Failed to configure endpoint for interface %d, error %d\n", interface_num, result);
        return result;
    }
    
    // Update the HID report descriptor for this interface
    if (interface_num < sizeof(hid_configs)/sizeof(hid_configs[0])) {
        hurricane_device_update_report_descriptor((uint8_t*)report_descriptor, descriptor_length);
        hid_configs[interface_num].in_endpoint = ep_address;
        hid_configs[interface_num].configured = true;
    }
    
    printf("Successfully configured HID interface %d\n", interface_num);
    return 0;
}

/**
 * Handle HID class control requests
 */
static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length)
{
    // Handle HID class-specific requests
    if ((setup->bmRequestType & 0x60) == 0x20) { // Class request
        uint8_t interface_num = setup->wIndex & 0xFF;
        
        printf("HID control request 0x%02X for interface %d\n", setup->bRequest, interface_num);
        
        // GET_DESCRIPTOR request for HID report descriptor
        if (setup->bRequest == 0x06 && (setup->wValue >> 8) == 0x22) {
            if (interface_num < sizeof(hid_configs)/sizeof(hid_configs[0])) {
                memcpy(buffer, hid_configs[interface_num].report_descriptor, 
                       hid_configs[interface_num].report_descriptor_length);
                *length = hid_configs[interface_num].report_descriptor_length;
                return true;
            }
        }
        
        // Handle GET_REPORT request
        if (setup->bRequest == 0x01) { // GET_REPORT
            // Respond with a dummy report for now
            uint8_t dummy_report[8] = {0};
            memcpy(buffer, dummy_report, sizeof(dummy_report));
            *length = sizeof(dummy_report);
            return true;
        }

        // Handle SET_REPORT request (for keyboard LEDs)
        if (setup->bRequest == 0x09 && interface_num == 1) { // SET_REPORT for keyboard
            printf("Received SET_REPORT: LEDs state = 0x%02X\n", ((uint8_t*)buffer)[0]);
            return true;
        }
        
        // Handle SET_IDLE request
        if (setup->bRequest == 0x0A) { // SET_IDLE
            return true; // Accept the request
        }
        
        // Handle SET_PROTOCOL request
        if (setup->bRequest == 0x0B) { // SET_PROTOCOL
            uint8_t protocol = setup->wValue & 0xFF; // 0 = Boot Protocol, 1 = Report Protocol
            printf("SET_PROTOCOL: interface %d to protocol %d\n", interface_num, protocol);
            return true;
        }
    }
    
    // Not handled
    return false;
}

/**
 * Configuration change callback
 */
static void configuration_callback(uint8_t configuration)
{
    printf("USB device configuration changed to %d\n", configuration);
}

/**
 * Interface change callback
 */
static void interface_callback(uint8_t interface, uint8_t alt_setting)
{
    printf("USB interface %d changed to alternate setting %d\n", interface, alt_setting);
}

/**
 * Build a complete configuration descriptor with interfaces
 */
static uint8_t* build_configuration_descriptor(size_t* total_length)
{
    // Calculate the total length of the configuration descriptor
    const int num_interfaces = 2; // Mouse and keyboard
    const int interface_desc_size = 9; // Interface descriptor size
    const int hid_desc_size = 9; // HID descriptor size
    const int endpoint_desc_size = 7; // Endpoint descriptor size
    
    size_t config_desc_total_length = 
        sizeof(config_descriptor_template) + 
        (num_interfaces * (interface_desc_size + hid_desc_size + endpoint_desc_size));
    
    // Allocate memory for the complete descriptor
    uint8_t* desc = malloc(config_desc_total_length);
    if (!desc) {
        return NULL;
    }
    
    // Copy the template
    memcpy(desc, config_descriptor_template, sizeof(config_descriptor_template));
    
    // Update the total length
    desc[2] = config_desc_total_length & 0xFF;
    desc[3] = (config_desc_total_length >> 8) & 0xFF;
    
    // Update the number of interfaces
    desc[4] = num_interfaces;
    
    // Current position in the descriptor
    size_t pos = sizeof(config_descriptor_template);
    
    // Add each interface with its HID descriptor and endpoint
    for (int i = 0; i < num_interfaces; i++) {
        // Determine protocol based on interface number
        uint8_t protocol = (i == 0) ? 2 : 1; // 2 for mouse, 1 for keyboard
        
        // Interface descriptor
        uint8_t interface_desc[] = {
            9,              // bLength
            4,              // bDescriptorType (Interface)
            i,              // bInterfaceNumber
            0,              // bAlternateSetting
            1,              // bNumEndpoints
            3,              // bInterfaceClass (HID)
            1,              // bInterfaceSubClass (Boot Interface)
            protocol,       // bInterfaceProtocol
            0               // iInterface
        };
        memcpy(desc + pos, interface_desc, sizeof(interface_desc));
        pos += sizeof(interface_desc);
        
        // HID descriptor
        uint8_t hid_desc[] = {
            9,              // bLength
            0x21,           // bDescriptorType (HID)
            0x11, 0x01,     // bcdHID 1.11
            0,              // bCountryCode
            1,              // bNumDescriptors
            0x22,           // bDescriptorType (Report)
            0, 0            // wDescriptorLength (placeholder)
        };
        
        // Set report descriptor length based on interface type
        uint16_t report_len = (i == 0) ? 
            sizeof(hid_mouse_report_descriptor) : 
            sizeof(hid_keyboard_report_descriptor);
            
        hid_desc[7] = report_len & 0xFF;
        hid_desc[8] = (report_len >> 8) & 0xFF;
        
        memcpy(desc + pos, hid_desc, sizeof(hid_desc));
        pos += sizeof(hid_desc);
        
        // Endpoint descriptor
        uint8_t ep_desc[] = {
            7,              // bLength
            5,              // bDescriptorType (Endpoint)
            0x80 | (i+1),   // bEndpointAddress (IN Endpoint)
            3,              // bmAttributes (Interrupt)
            64, 0,          // wMaxPacketSize
            10              // bInterval (10ms)
        };
        memcpy(desc + pos, ep_desc, sizeof(ep_desc));
        pos += sizeof(ep_desc);
    }
    
    *total_length = config_desc_total_length;
    return desc;
}