#include "usb_host_controller.h"
#include "hw/hurricane_hw_hal.h"
#include "usb/usb_control.h"
#include "usb/usb_hid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// C99 boolean support
#include <stdbool.h>

static usb_device_t device;

// Buffer for configuration descriptor
static uint8_t config_buffer[256];

// Forward declaration of helper functions
static int usb_get_config_descriptor(uint8_t config_index);
static int usb_parse_configuration(uint8_t* buffer, uint16_t len);
static int usb_set_configuration(uint8_t config_value);
static int usb_find_hid_interface(uint8_t* buffer, uint16_t len, uint8_t* interface_num, uint8_t* endpoint_addr);

void usb_host_init(void)
{
    device.state = kUSB_DeviceStateConfigured;
    device.device_address = 0;
    
    hurricane_hw_reset_bus(); // Reset the USB bus
    printf("[host] Bus reset initiated\n");
}

void usb_host_poll(void)
{
    switch (device.state)
    {
        case kUSB_DeviceStateDefault:
            printf("[host] Setting device address...\n");
            if (usb_control_set_address(1) != 0) {
                printf("[host] Error setting device address.\n");
                break;
            }
            device.device_address = 1;
            device.state = kUSB_DeviceStateAddress;
            break;

        case kUSB_DeviceStateAddress:
            printf("[host] Fetching device descriptor...\n");
            if (usb_control_get_device_descriptor(device.device_address, &device.device_desc) != 0) {
                printf("[host] Error fetching device descriptor.\n");
                break;
            }
            
            // Get first configuration descriptor (index 0)
            printf("[host] Fetching configuration descriptor...\n");
            if (usb_get_config_descriptor(0) != 0) {
                printf("[host] Error fetching configuration descriptor.\n");
                break;
            }
            
            // Set configuration (usually 1 is the default)
            printf("[host] Setting configuration...\n");
            if (usb_set_configuration(1) != 0) {
                printf("[host] Error setting configuration.\n");
                break;
            }
            
            device.state = kUSB_DeviceStateConfigured;
            break;

        case kUSB_DeviceStateConfigured:
            // Device is configured, handle class-specific tasks
            // If HID interface was found, poll it
            if (device.hid_configured) {
                // Get HID device handle from our device table
                hurricane_device_t* dev = hurricane_get_device(0);
                if (dev && dev->is_active && dev->hid_device) {
                    hurricane_hid_task(dev);
                }
            }
            break;

        default:
            printf("[host] Device in error state. Resetting...\n");
            hurricane_hw_reset_bus();
            device.state = kUSB_DeviceStateDefault;
            break;
    }
}

// Helper to fetch and process configuration descriptor
static int usb_get_config_descriptor(uint8_t config_index)
{
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x80,  // Device to Host, Standard, Device
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_CONFIGURATION << 8) | config_index,
        .wIndex = 0,
        .wLength = 9  // Initially just get the header to determine total size
    };
    
    memset(config_buffer, 0, sizeof(config_buffer));
    
    // First, get just the configuration descriptor header (9 bytes)
    if (hurricane_hw_control_transfer(&setup, config_buffer, 9) < 9) {
        printf("[host] Failed to get configuration descriptor header\n");
        return -1;
    }
    
    // Parse the configuration descriptor to get the total length
    usb_config_descriptor_t config_desc;
    if (usb_parse_config_descriptor(config_buffer, &config_desc) != 0) {
        printf("[host] Failed to parse configuration descriptor\n");
        return -1;
    }
    
    uint16_t total_length = config_desc.wTotalLength;
    printf("[host] Configuration descriptor total length: %u bytes\n", total_length);
    
    if (total_length > sizeof(config_buffer)) {
        printf("[host] Configuration descriptor too large for buffer\n");
        total_length = sizeof(config_buffer);
    }
    
    // Now get the complete configuration descriptor with all interfaces and endpoints
    setup.wLength = total_length;
    if (hurricane_hw_control_transfer(&setup, config_buffer, total_length) < total_length) {
        printf("[host] Failed to get complete configuration descriptor\n");
        return -1;
    }
    
    // Process the configuration to find interfaces and endpoints
    return usb_parse_configuration(config_buffer, total_length);
}

// Parse the configuration descriptor and its embedded interface/endpoint descriptors
static int usb_parse_configuration(uint8_t* buffer, uint16_t len)
{
    uint8_t hid_interface = 0;
    uint8_t hid_endpoint = 0;
    device.hid_configured = 0;
    
    if (usb_find_hid_interface(buffer, len, &hid_interface, &hid_endpoint)) {
        printf("[host] Found HID interface %d with interrupt endpoint 0x%02X\n", 
                hid_interface, hid_endpoint);
        
        // Store the HID interface info
        device.hid_interface = hid_interface;
        device.hid_endpoint = hid_endpoint;
        device.hid_configured = 1;
        
        // Create and configure HID device
        hurricane_device_t* dev = hurricane_get_device(0);
        if (dev && dev->is_active) {
            // Allocate HID device structure if not already allocated
            if (!dev->hid_device) {
                dev->hid_device = malloc(sizeof(hurricane_hid_device_t));
                if (!dev->hid_device) {
                    printf("[host] Failed to allocate HID device structure\n");
                    return -1;
                }
                memset(dev->hid_device, 0, sizeof(hurricane_hid_device_t));
            }
            
            // Initialize the HID device
            dev->hid_device->interface_number = hid_interface;
            hurricane_hid_init(dev);
            
            // Attempt to fetch HID report descriptor
            hurricane_hid_fetch_report_descriptor(dev);
            
            printf("[host] HID device configured successfully\n");
        }
    } else {
        printf("[host] No HID interface found in configuration\n");
    }
    
    return 0;
}

// Helper to set the device configuration
static int usb_set_configuration(uint8_t config_value)
{
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x00,  // Host to Device, Standard, Device
        .bRequest = 0x09,       // SET_CONFIGURATION
        .wValue = config_value,
        .wIndex = 0,
        .wLength = 0
    };
    
    if (hurricane_hw_control_transfer(&setup, NULL, 0) != 0) {
        printf("[host] Failed to set configuration %d\n", config_value);
        return -1;
    }
    
    printf("[host] Device configured with configuration %d\n", config_value);
    return 0;
}

// Helper function to find HID interface and endpoint in configuration descriptor
static int usb_find_hid_interface(uint8_t* buffer, uint16_t len, uint8_t* interface_num, uint8_t* endpoint_addr)
{
    uint16_t pos = 0;
    bool found_hid = false;
    uint8_t current_interface = 0;
    
    while (pos < len) {
        uint8_t desc_len = buffer[pos];
        uint8_t desc_type = buffer[pos + 1];
        
        if (desc_len == 0) {
            // Invalid descriptor
            break;
        }
        
        if (desc_type == USB_DESC_TYPE_INTERFACE) {
            // Interface descriptor
            current_interface = buffer[pos + 2];  // bInterfaceNumber
            uint8_t interface_class = buffer[pos + 5];  // bInterfaceClass
            uint8_t interface_subclass = buffer[pos + 6];  // bInterfaceSubClass
            uint8_t interface_protocol = buffer[pos + 7];  // bInterfaceProtocol
            
            if (interface_class == 3) {  // HID class
                printf("[host] Found HID interface %d (subclass: %d, protocol: %d)\n", 
                       current_interface, interface_subclass, interface_protocol);
                *interface_num = current_interface;
                found_hid = true;
                
                // HID protocol values:
                // 1 = Keyboard
                // 2 = Mouse
                if (interface_protocol == 2) {
                    printf("[host] HID device is a mouse\n");
                } else if (interface_protocol == 1) {
                    printf("[host] HID device is a keyboard\n");
                }
            }
        } else if (desc_type == USB_DESC_TYPE_ENDPOINT && found_hid) {
            // Found an endpoint for the current HID interface
            uint8_t endpoint = buffer[pos + 2];  // bEndpointAddress
            uint8_t attributes = buffer[pos + 3];  // bmAttributes
            
            // Check if it's an interrupt endpoint (type = 3) and IN direction (bit 7 set)
            if ((attributes & 0x03) == 3 && (endpoint & 0x80)) {
                printf("[host] Found interrupt IN endpoint: 0x%02X\n", endpoint);
                *endpoint_addr = endpoint;
                return 1;  // Success
            }
        }
        
        pos += desc_len;  // Move to next descriptor
    }
    
    return 0;  // Not found
}
