/**
 * @file device_config.c
 * @brief Implementation of device mode configuration for LPC55S69 dual HID example
 * 
 * This file implements the device mode functionality for the USB0 controller
 * of the LPC55S69 platform.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "device_config.h"
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
static const uint8_t device_descriptor[] = {
    18,                // bLength
    1,                 // bDescriptorType (Device)
    0x00, 0x02,        // bcdUSB 2.0
    0,                 // bDeviceClass (Use class info from Interface Descriptors)
    0,                 // bDeviceSubClass
    0,                 // bDeviceProtocol
    64,                // bMaxPacketSize0
    0xC0, 0x16,        // idVendor (Hurricane)
    0xdc, 0x05,        // idProduct (Dual HID Example)
    0x01, 0x01,        // bcdDevice
    1,                 // iManufacturer (String index)
    2,                 // iProduct (String index)
    3,                 // iSerialNumber (String index)
    1                  // bNumConfigurations
};

// String descriptors
static const uint8_t string_descriptor_0[] = {
    4,                 // bLength
    3,                 // bDescriptorType (String)
    0x09, 0x04         // Language ID (English US)
};

static const uint8_t string_descriptor_1[] = {
    18,                // bLength
    3,                 // bDescriptorType (String)
    'H', 0, 'u', 0, 'r', 0, 'r', 0, 'i', 0, 'c', 0, 'a', 0, 'n', 0
};

static const uint8_t string_descriptor_2[] = {
    40,                // bLength
    3,                 // bDescriptorType (String)
    'L', 0, 'P', 0, 'C', 0, '5', 0, '5', 0, 'S', 0, '6', 0, '9', 0, ' ', 0,
    'D', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0, 'D', 0
};

static const uint8_t string_descriptor_3[] = {
    26,                // bLength
    3,                 // bDescriptorType (String)
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0, '9', 0, '0', 0, '0', 0, '1', 0
};

// HID interface configuration
static hid_config_t hid_configs[] = {
    { 0, hid_mouse_report_descriptor, sizeof(hid_mouse_report_descriptor), 0x81, 2, false }, // Mouse
    { 1, hid_keyboard_report_descriptor, sizeof(hid_keyboard_report_descriptor), 0x82, 1, false } // Keyboard
};

// State tracking
static bool device_connected = false;
static bool interfaces_configured = false;

// Callback function for HID control requests
static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);

// Function to build a complete configuration descriptor with interfaces
static uint8_t* build_configuration_descriptor(size_t* total_length);

// Add a HID interface at runtime
static int add_hid_interface(uint8_t interface_num, const uint8_t* report_descriptor, uint16_t descriptor_length, uint8_t protocol);

/**
 * Initialize the device-mode configuration
 */
void device_config_init(void)
{
    printf("[LPC55S69-Device Config] Initializing device configuration\n");
    
    // Build the initial configuration descriptor
    size_t config_desc_len;
    uint8_t* config_desc = build_configuration_descriptor(&config_desc_len);
    
    if (!config_desc) {
        printf("[LPC55S69-Device Config] Failed to build configuration descriptor!\n");
        return;
    }
    
    // Set up the device descriptors
    hurricane_device_descriptors_t descriptors = {0};
    descriptors.device_descriptor = (uint8_t*)device_descriptor;
    descriptors.device_descriptor_length = sizeof(device_descriptor);
    descriptors.config_descriptor = config_desc;
    descriptors.config_descriptor_length = config_desc_len;
    
    // Add string descriptors
    descriptors.string_descriptors[0] = (uint8_t*)string_descriptor_0;
    descriptors.string_descriptor_lengths[0] = sizeof(string_descriptor_0);
    descriptors.string_descriptors[1] = (uint8_t*)string_descriptor_1;
    descriptors.string_descriptor_lengths[1] = sizeof(string_descriptor_1);
    descriptors.string_descriptors[2] = (uint8_t*)string_descriptor_2;
    descriptors.string_descriptor_lengths[2] = sizeof(string_descriptor_2);
    descriptors.string_descriptors[3] = (uint8_t*)string_descriptor_3;
    descriptors.string_descriptor_lengths[3] = sizeof(string_descriptor_3);
    
    // Update device descriptors
    hurricane_device_update_descriptors(&descriptors);
    
    // Free the dynamically allocated descriptor
    free(config_desc);
    
    // Register configuration/interface change callbacks in main.c
    
    printf("[LPC55S69-Device Config] Device configuration initialized\n");
}

/**
 * Deinitialize device-mode configuration
 */
void device_config_deinit(void)
{
    printf("[LPC55S69-Device Config] Deinitializing device configuration\n");
    
    // Reset interface state
    for (int i = 0; i < sizeof(hid_configs)/sizeof(hid_configs[0]); i++) {
        hid_configs[i].configured = false;
    }
    
    interfaces_configured = false;
    device_connected = false;
    
    printf("[LPC55S69-Device Config] Device configuration deinitialized\n");
}

/**
 * Run device-mode tasks
 */
void device_config_task(void)
{
    static uint32_t last_mouse_time = 0;
    static uint32_t last_keyboard_time = 0;
    uint32_t current_time;
    
    // Check if host is connected
    bool current_connection = hurricane_hw_device_host_connected();
    
    // Connection state change detection
    if (current_connection != device_connected) {
        if (current_connection) {
            printf("[LPC55S69-Device Config] Host connected to device (USB0)\n");
            // Configure interfaces when host connects
            if (!interfaces_configured) {
                // Add Mouse HID interface
                add_hid_interface(
                    hid_configs[0].interface_num,
                    hid_configs[0].report_descriptor,
                    hid_configs[0].report_descriptor_length,
                    hid_configs[0].protocol
                );
                
                // Add Keyboard HID interface
                add_hid_interface(
                    hid_configs[1].interface_num,
                    hid_configs[1].report_descriptor,
                    hid_configs[1].report_descriptor_length,
                    hid_configs[1].protocol
                );
                
                interfaces_configured = true;
                
                // Trigger device reset to apply new configuration
                hurricane_device_trigger_reset();
                
                printf("[LPC55S69-Device Config] HID interfaces configured\n");
            }
        } else {
            printf("[LPC55S69-Device Config] Host disconnected from device\n");
        }
        device_connected = current_connection;
    }
    
    // Only send reports if connected and configured
    if (device_connected && interfaces_configured) {
        current_time = hurricane_get_time_ms();
        
        // Generate periodic mouse movements if mouse interface is configured
        if (hid_configs[0].configured && (current_time - last_mouse_time > 1000)) {
            // Move cursor in a circular pattern
            static int angle = 0;
            int8_t dx = (int8_t)(10 * cos(angle * 3.14159 / 180));
            int8_t dy = (int8_t)(10 * sin(angle * 3.14159 / 180));
            angle = (angle + 15) % 360;
            
            device_config_send_mouse_report(dx, dy, 0);
            last_mouse_time = current_time;
        }
        
        // Generate periodic keyboard key presses if keyboard interface is configured
        if (hid_configs[1].configured && (current_time - last_keyboard_time > 3000)) {
            // Type "HELLO" one character at a time
            static int char_index = 0;
            static const uint8_t keycodes[] = {0x0B, 0x08, 0x0F, 0x0F, 0x12}; // H-E-L-L-O
            static const char *chars = "HELLO";
            
            printf("[LPC55S69-Device Config] Sending keyboard report for '%c'\n", chars[char_index]);
            
            // Send key press
            uint8_t report_keycodes[6] = {0};
            report_keycodes[0] = keycodes[char_index];
            device_config_send_keyboard_report(0, report_keycodes);
            
            // Short delay
            hurricane_delay_ms(50);
            
            // Send key release
            memset(report_keycodes, 0, sizeof(report_keycodes));
            device_config_send_keyboard_report(0, report_keycodes);
            
            char_index = (char_index + 1) % 5;
            last_keyboard_time = current_time;
        }
    }
}

/**
 * Handle USB device configuration changes
 */
void device_config_set_configuration_callback(uint8_t configuration)
{
    printf("[LPC55S69-Device Config] Device configuration set to %d\n", configuration);
}

/**
 * Handle USB interface setting changes
 */
void device_config_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
    printf("[LPC55S69-Device Config] Interface %d alternate setting changed to %d\n", interface, alt_setting);
}

/**
 * Check if device has an active connection to a host
 */
bool device_config_is_connected(void)
{
    return device_connected;
}

/**
 * Generate and send a mouse movement report
 */
int device_config_send_mouse_report(int8_t dx, int8_t dy, uint8_t buttons)
{
    if (!device_connected || !hid_configs[0].configured) {
        return -1;
    }
    
    uint8_t mouse_report[] = {
        buttons,    // Button state
        dx,         // X movement
        dy          // Y movement
    };
    
    int result = hurricane_hw_device_interrupt_in_transfer(
        hid_configs[0].in_endpoint,
        mouse_report,
        sizeof(mouse_report)
    );
    
    if (result != 0) {
        printf("[LPC55S69-Device Config] Failed to send mouse report, error %d\n", result);
    }
    
    return result;
}

/**
 * Generate and send a keyboard report
 */
int device_config_send_keyboard_report(uint8_t modifier, const uint8_t keycodes[6])
{
    if (!device_connected || !hid_configs[1].configured) {
        return -1;
    }
    
    uint8_t keyboard_report[8];
    keyboard_report[0] = modifier;
    keyboard_report[1] = 0; // Reserved byte
    memcpy(keyboard_report + 2, keycodes, 6);
    
    int result = hurricane_hw_device_interrupt_in_transfer(
        hid_configs[1].in_endpoint,
        keyboard_report,
        sizeof(keyboard_report)
    );
    
    if (result != 0) {
        printf("[LPC55S69-Device Config] Failed to send keyboard report, error %d\n", result);
    }
    
    return result;
}

/**
 * Get the current mouse HID configuration
 */
const hid_config_t* device_config_get_mouse_config(void)
{
    return &hid_configs[0];
}

/**
 * Get the current keyboard HID configuration
 */
const hid_config_t* device_config_get_keyboard_config(void)
{
    return &hid_configs[1];
}

/**
 * Handle HID class control requests
 */
static bool hid_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length)
{
    // Handle HID class-specific requests
    if ((setup->bmRequestType & 0x60) == 0x20) { // Class request
        uint8_t interface_num = setup->wIndex & 0xFF;
        
        printf("[LPC55S69-Device Config] HID control request 0x%02X for interface %d\n", setup->bRequest, interface_num);
        
        // GET_DESCRIPTOR request for HID report descriptor
        if (setup->bRequest == 0x06 && (setup->wValue >> 8) == 0x22) {
            for (int i = 0; i < sizeof(hid_configs)/sizeof(hid_configs[0]); i++) {
                if (hid_configs[i].interface_num == interface_num) {
                    memcpy(buffer, hid_configs[i].report_descriptor, hid_configs[i].report_descriptor_length);
                    *length = hid_configs[i].report_descriptor_length;
                    return true;
                }
            }
        }
        
        // Handle GET_REPORT request
        if (setup->bRequest == 0x01) { // GET_REPORT
            uint8_t report_type = setup->wValue >> 8;
            uint8_t report_id = setup->wValue & 0xFF;
            
            printf("[LPC55S69-Device Config] GET_REPORT: type %d, id %d\n", report_type, report_id);
            
            // Determine which interface is being queried
            if (interface_num == hid_configs[0].interface_num) { // Mouse
                uint8_t mouse_report[] = {0x00, 0x00, 0x00}; // No buttons, no movement
                memcpy(buffer, mouse_report, sizeof(mouse_report));
                *length = sizeof(mouse_report);
                return true;
            } else if (interface_num == hid_configs[1].interface_num) { // Keyboard
                uint8_t keyboard_report[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // No keys pressed
                memcpy(buffer, keyboard_report, sizeof(keyboard_report));
                *length = sizeof(keyboard_report);
                return true;
            }
        }
        
        // Handle SET_REPORT request
        if (setup->bRequest == 0x09) { // SET_REPORT
            // Process incoming report from host
            return true;
        }
        
        // Handle SET_IDLE request
        if (setup->bRequest == 0x0A) { // SET_IDLE
            return true; // Accept the request
        }
        
        // Handle SET_PROTOCOL request
        if (setup->bRequest == 0x0B) { // SET_PROTOCOL
            uint8_t protocol = setup->wValue & 0xFF; // 0 = Boot Protocol, 1 = Report Protocol
            printf("[LPC55S69-Device Config] SET_PROTOCOL: interface %d to protocol %d\n", interface_num, protocol);
            return true;
        }
    }
    
    // Not handled
    return false;
}

/**
 * Add a HID interface at runtime
 */
static int add_hid_interface(uint8_t interface_num, const uint8_t* report_descriptor, uint16_t descriptor_length, uint8_t protocol)
{
    printf("[LPC55S69-Device Config] Adding HID interface %d (protocol %d)\n", interface_num, protocol);
    
    // Create interface descriptor
    hurricane_interface_descriptor_t interface_desc = {
        .interface_num = interface_num,
        .interface_class = 0x03,         // HID Class
        .interface_subclass = 0x01,      // Boot interface subclass
        .interface_protocol = protocol,  // 1=Keyboard, 2=Mouse
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
        printf("[LPC55S69-Device Config] Failed to add interface %d, error %d\n", interface_num, result);
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
        printf("[LPC55S69-Device Config] Failed to configure endpoint for interface %d, error %d\n", interface_num, result);
        return result;
    }
    
    // Update the HID report descriptor for this interface
    for (int i = 0; i < sizeof(hid_configs)/sizeof(hid_configs[0]); i++) {
        if (hid_configs[i].interface_num == interface_num) {
            hurricane_device_update_report_descriptor((uint8_t*)report_descriptor, descriptor_length);
            hid_configs[i].in_endpoint = ep_address;
            hid_configs[i].configured = true;
            break;
        }
    }
    
    printf("[LPC55S69-Device Config] Successfully configured HID interface %d\n", interface_num);
    return 0;
}

/**
 * Build a complete configuration descriptor with interfaces
 */
static uint8_t* build_configuration_descriptor(size_t* total_length)
{
    // Config descriptor template
    const uint8_t config_descriptor_template[] = {
        9,                 // bLength
        2,                 // bDescriptorType (Configuration)
        0, 0,              // wTotalLength - will be filled in
        0,                 // bNumInterfaces - will be filled in
        1,                 // bConfigurationValue
        0,                 // iConfiguration (String Index)
        0x80,              // bmAttributes (Bus Powered)
        50,                // bMaxPower (100mA)
    };

    // Calculate the total length of the configuration descriptor
    const int num_interfaces = sizeof(hid_configs)/sizeof(hid_configs[0]); // Mouse and keyboard
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
        uint8_t protocol = hid_configs[i].protocol;
        
        // Interface descriptor
        uint8_t interface_desc[] = {
            9,              // bLength
            4,              // bDescriptorType (Interface)
            i,              // bInterfaceNumber
            0,              // bAlternateSetting
            1,              // bNumEndpoints
            3,              // bInterfaceClass (HID)
            1,              // bInterfaceSubClass (Boot Interface)
            protocol,       // bInterfaceProtocol (1=Keyboard, 2=Mouse)
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
        
        // Set report descriptor length
        uint16_t report_len = hid_configs[i].report_descriptor_length;
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