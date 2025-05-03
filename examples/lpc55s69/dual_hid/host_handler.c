/**
 * @file host_handler.c
 * @brief Implementation of host mode handler for LPC55S69 dual HID example
 * 
 * This file implements the host mode functionality for the USB1 controller
 * of the LPC55S69 platform.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_handler.h"
#include "hurricane.h"
#include "core/usb_interface_manager.h"

// Maximum number of devices that can be tracked
#define MAX_USB_DEVICES 4

// HID-specific request codes
#define USB_HID_GET_REPORT      0x01
#define USB_HID_GET_IDLE        0x02
#define USB_HID_GET_PROTOCOL    0x03
#define USB_HID_SET_REPORT      0x09
#define USB_HID_SET_IDLE        0x0A
#define USB_HID_SET_PROTOCOL    0x0B

// Timeout for operations in milliseconds
#define USB_TIMEOUT_MS 100

// Device tracking
static usb_device_info_t devices[MAX_USB_DEVICES] = {0};
static int active_device_idx = -1;
static int num_devices = 0;

// Report buffer and tracking
static hid_report_data_t last_report = {0};
static bool new_report_available = false;

// Report callback
static void (*report_callback)(const hid_report_data_t* report) = NULL;

// Host mode callbacks
static void host_device_attached_callback(void* device_handle);
static void host_device_detached_callback(void* device_handle);
static bool host_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);
static void host_data_callback(uint8_t endpoint, void* buffer, uint16_t length);

// Helper functions
static bool enumerate_device(void* device_handle, usb_device_info_t* device_info);
static bool configure_hid_device(usb_device_info_t* device_info);
static void process_hid_report(const uint8_t* report_data, uint16_t length, uint8_t endpoint);
static void print_device_info(const usb_device_info_t* device_info);

/**
 * Initialize the host-mode handler
 */
void host_handler_init(void)
{
    printf("[LPC55S69-Host Handler] Initializing USB host handler (USB1)\n");
    
    // Clear device tracking
    memset(devices, 0, sizeof(devices));
    active_device_idx = -1;
    num_devices = 0;
    
    // Clear report buffer
    memset(&last_report, 0, sizeof(last_report));
    new_report_available = false;
    
    // Register our host class handler for HID devices (class 3)
    hurricane_host_class_handler_t hid_host_handler = {
        .match_callback = NULL,  // Default matching for HID class
        .attach_callback = host_device_attached_callback,
        .detach_callback = host_device_detached_callback,
        .control_callback = host_control_callback,
        .data_callback = host_data_callback
    };
    
    hurricane_register_host_class_handler(3, 0, 0, &hid_host_handler);
    
    printf("[LPC55S69-Host Handler] Registered HID device handler\n");
}

/**
 * Deinitialize the host-mode handler
 */
void host_handler_deinit(void)
{
    printf("[LPC55S69-Host Handler] Deinitializing USB host handler\n");
    
    // Unregister our HID host handler
    hurricane_unregister_host_class_handler(3, 0, 0);
    
    // Clear device tracking
    memset(devices, 0, sizeof(devices));
    active_device_idx = -1;
    num_devices = 0;
    
    // Clear report buffer
    memset(&last_report, 0, sizeof(last_report));
    new_report_available = false;
    
    // Clear callback
    report_callback = NULL;
}

/**
 * Run host-mode tasks
 */
void host_handler_task(void)
{
    // Check for connected devices if none are active
    if (active_device_idx < 0) {
        if (hurricane_hw_host_device_connected()) {
            printf("[LPC55S69-Host Handler] Device detected but not yet enumerated\n");
            // Device will be enumerated by attach callback
        }
    }
    else {
        // Device is connected, check if we should poll for data
        if (devices[active_device_idx].connected && 
            devices[active_device_idx].is_hid && 
            devices[active_device_idx].endpoint_in != 0) {
            
            // For HID devices, we don't need to manually poll since
            // the USB host controller will do that for us and call
            // our data callback when data arrives
        }
    }
    
    // Process any pending events, will trigger callbacks if needed
    hurricane_hw_host_poll();
}

/**
 * Check if a device is attached to the host controller
 */
bool host_handler_is_device_connected(void)
{
    return (active_device_idx >= 0 && devices[active_device_idx].connected);
}

/**
 * Get information about the currently connected device
 */
bool host_handler_get_device_info(usb_device_info_t* device_info)
{
    if (!device_info || active_device_idx < 0 || !devices[active_device_idx].connected) {
        return false;
    }
    
    // Copy device information to provided structure
    memcpy(device_info, &devices[active_device_idx], sizeof(usb_device_info_t));
    return true;
}

/**
 * Read a HID report from the connected device
 */
bool host_handler_read_hid_report(hid_report_data_t* report)
{
    if (!report || !new_report_available) {
        return false;
    }
    
    // Copy report data to provided structure
    memcpy(report, &last_report, sizeof(hid_report_data_t));
    
    // Clear the new report flag
    new_report_available = false;
    
    return true;
}

/**
 * Send a HID report to the connected device
 */
bool host_handler_send_hid_report(uint8_t report_id, const uint8_t* data, uint16_t length)
{
    if (!data || length == 0 || active_device_idx < 0 || !devices[active_device_idx].connected) {
        return false;
    }
    
    if (!devices[active_device_idx].is_hid || devices[active_device_idx].endpoint_out == 0) {
        printf("[LPC55S69-Host Handler] Device is not HID or has no OUT endpoint\n");
        return false;
    }
    
    // Allocate buffer for report including report ID if needed
    uint8_t* report_buffer = NULL;
    uint16_t report_length = 0;
    
    if (report_id != 0) {
        report_buffer = (uint8_t*)malloc(length + 1);
        if (!report_buffer) {
            printf("[LPC55S69-Host Handler] Failed to allocate report buffer\n");
            return false;
        }
        
        report_buffer[0] = report_id;
        memcpy(report_buffer + 1, data, length);
        report_length = length + 1;
    } else {
        // No report ID, use data directly
        report_buffer = (uint8_t*)data;
        report_length = length;
    }
    
    // Send the report
    int result = hurricane_hw_host_interrupt_out_transfer(
        devices[active_device_idx].endpoint_out,
        report_buffer,
        report_length
    );
    
    if (report_id != 0) {
        free(report_buffer);
    }
    
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to send HID report, error %d\n", result);
        return false;
    }
    
    return true;
}

/**
 * Set the LED states for a HID keyboard
 */
bool host_handler_set_keyboard_leds(uint8_t leds)
{
    if (active_device_idx < 0 || !devices[active_device_idx].connected) {
        return false;
    }
    
    if (!devices[active_device_idx].is_hid) {
        printf("[LPC55S69-Host Handler] Device is not a HID device\n");
        return false;
    }
    
    // Create setup packet for SET_REPORT request
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x21,  // Host-to-device, class, interface
        .bRequest = USB_HID_SET_REPORT,
        .wValue = 0x0200,       // Output report, ID 0
        .wIndex = devices[active_device_idx].current_interface,
        .wLength = 1
    };
    
    // Send control transfer with LED state
    int result = hurricane_hw_host_control_transfer(
        &setup,
        &leds,
        1
    );
    
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to set keyboard LEDs, error %d\n", result);
        return false;
    }
    
    return true;
}

/**
 * Register callback function for HID report reception
 */
void host_handler_register_report_callback(void (*callback)(const hid_report_data_t* report))
{
    report_callback = callback;
}

/**
 * Get the HID report descriptor from the connected device
 */
uint16_t host_handler_get_report_descriptor(uint8_t* descriptor, uint16_t max_length)
{
    if (!descriptor || max_length == 0 || active_device_idx < 0 || !devices[active_device_idx].connected) {
        return 0;
    }
    
    if (!devices[active_device_idx].is_hid) {
        printf("[LPC55S69-Host Handler] Device is not a HID device\n");
        return 0;
    }
    
    // Create setup packet for GET_DESCRIPTOR (HID Report Descriptor)
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x81,  // Device-to-host, standard, interface
        .bRequest = 0x06,       // GET_DESCRIPTOR
        .wValue = 0x2200,       // Report descriptor, index 0
        .wIndex = devices[active_device_idx].current_interface,
        .wLength = max_length
    };
    
    // Send control transfer to get descriptor
    int result = hurricane_hw_host_control_transfer(
        &setup,
        descriptor,
        max_length
    );
    
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to get HID report descriptor, error %d\n", result);
        return 0;
    }
    
    return (uint16_t)result;
}

/**
 * Host mode callback for device attachment
 */
static void host_device_attached_callback(void* device_handle)
{
    printf("[LPC55S69-Host Handler] USB device attached to USB1\n");
    
    // Find an empty slot to store device information
    int device_idx = -1;
    for (int i = 0; i < MAX_USB_DEVICES; i++) {
        if (!devices[i].connected) {
            device_idx = i;
            break;
        }
    }
    
    if (device_idx < 0) {
        printf("[LPC55S69-Host Handler] No free slots to track device\n");
        return;
    }
    
    // Enumerate the device
    usb_device_info_t device_info = {0};
    if (!enumerate_device(device_handle, &device_info)) {
        printf("[LPC55S69-Host Handler] Failed to enumerate device\n");
        return;
    }
    
    // Store device information
    memcpy(&devices[device_idx], &device_info, sizeof(usb_device_info_t));
    devices[device_idx].connected = true;
    
    // Make this the active device
    active_device_idx = device_idx;
    num_devices++;
    
    // Print device information
    print_device_info(&devices[device_idx]);
    
    // If it's a HID device, configure it
    if (devices[device_idx].is_hid) {
        if (!configure_hid_device(&devices[device_idx])) {
            printf("[LPC55S69-Host Handler] Failed to configure HID device\n");
        }
    }
}

/**
 * Host mode callback for device detachment
 */
static void host_device_detached_callback(void* device_handle)
{
    printf("[LPC55S69-Host Handler] USB device detached from USB1\n");
    
    // Find the device in our tracking array
    for (int i = 0; i < MAX_USB_DEVICES; i++) {
        if (devices[i].connected) {
            // Mark as disconnected
            devices[i].connected = false;
            
            if (i == active_device_idx) {
                active_device_idx = -1; // Clear active device
            }
            
            num_devices--;
            break;
        }
    }
    
    // If we have other devices, make one of them active
    if (num_devices > 0 && active_device_idx < 0) {
        for (int i = 0; i < MAX_USB_DEVICES; i++) {
            if (devices[i].connected) {
                active_device_idx = i;
                break;
            }
        }
    }
}

/**
 * Host mode callback for control transfers
 */
static bool host_control_callback(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length)
{
    if (!setup) {
        return false;
    }
    
    // We typically don't need to handle control transfers directly
    // as the USB stack takes care of standard requests
    
    return false; // Not handled by us
}

/**
 * Host mode callback for data reception
 */
static void host_data_callback(uint8_t endpoint, void* buffer, uint16_t length)
{
    printf("[LPC55S69-Host Handler] Received %d bytes on endpoint 0x%02X\n", length, endpoint);
    
    // Check if this is data from our active device
    if (active_device_idx >= 0 && devices[active_device_idx].connected) {
        if (devices[active_device_idx].endpoint_in == endpoint) {
            // Process HID report data
            process_hid_report((const uint8_t*)buffer, length, endpoint);
        }
    }
}

/**
 * Enumerate a USB device to get its information
 */
static bool enumerate_device(void* device_handle, usb_device_info_t* device_info)
{
    if (!device_handle || !device_info) {
        return false;
    }
    
    memset(device_info, 0, sizeof(usb_device_info_t));
    
    // Get device descriptor
    uint8_t device_descriptor[18]; // Standard USB device descriptor size
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x80,  // Device-to-host, standard, device
        .bRequest = 0x06,       // GET_DESCRIPTOR
        .wValue = 0x0100,       // Device descriptor, index 0
        .wIndex = 0,
        .wLength = sizeof(device_descriptor)
    };
    
    int result = hurricane_hw_host_control_transfer(
        &setup,
        device_descriptor,
        sizeof(device_descriptor)
    );
    
    if (result < 18) {
        printf("[LPC55S69-Host Handler] Failed to get device descriptor, error %d\n", result);
        return false;
    }
    
    // Extract information from device descriptor
    device_info->vendor_id = (device_descriptor[8] | (device_descriptor[9] << 8));
    device_info->product_id = (device_descriptor[10] | (device_descriptor[11] << 8));
    device_info->device_class = device_descriptor[4];
    device_info->device_subclass = device_descriptor[5];
    device_info->device_protocol = device_descriptor[6];
    
    // Check if this is a HID class device at device level
    if (device_info->device_class == 0x03) {
        device_info->is_hid = true;
    }
    
    // Get string descriptors (manufacturer, product, serial)
    if (device_descriptor[14] > 0) { // Product string index
        uint8_t string_descriptor[64];
        setup.wValue = 0x0300 | device_descriptor[14]; // String descriptor, index from device descriptor
        
        result = hurricane_hw_host_control_transfer(
            &setup,
            string_descriptor,
            sizeof(string_descriptor)
        );
        
        if (result >= 2) {
            // Convert from Unicode to ASCII (simple conversion)
            uint8_t str_len = string_descriptor[0];
            uint8_t max_chars = (str_len - 2) / 2; // Skip header and convert UTF-16 to ASCII
            max_chars = (max_chars >= sizeof(device_info->product_name)) ? 
                      sizeof(device_info->product_name) - 1 : max_chars;
            
            for (int i = 0; i < max_chars; i++) {
                device_info->product_name[i] = string_descriptor[2 + (i * 2)]; // Skip UTF-16 high byte
            }
            device_info->product_name[max_chars] = '\0';
        }
    }
    
    // Get configuration descriptor to check interfaces
    uint8_t config_descriptor[9]; // Just the header first
    setup.wValue = 0x0200; // Configuration descriptor, index 0
    
    result = hurricane_hw_host_control_transfer(
        &setup,
        config_descriptor,
        sizeof(config_descriptor)
    );
    
    if (result >= 9) {
        uint16_t total_length = config_descriptor[2] | (config_descriptor[3] << 8);
        device_info->interface_count = config_descriptor[4];
        
        if (total_length > 9 && device_info->interface_count > 0) {
            // Get full configuration descriptor with interfaces
            uint8_t* full_config = (uint8_t*)malloc(total_length);
            if (full_config) {
                setup.wLength = total_length;
                
                result = hurricane_hw_host_control_transfer(
                    &setup,
                    full_config,
                    total_length
                );
                
                if (result >= total_length) {
                    // Parse interfaces to find HID interfaces if device is not HID class
                    if (!device_info->is_hid) {
                        uint16_t pos = 9; // Start after config descriptor header
                        
                        while (pos < total_length) {
                            if (pos + 9 > total_length) break; // Not enough data for an interface descriptor
                            
                            if (full_config[pos + 1] == 0x04) { // Interface descriptor
                                uint8_t intf_class = full_config[pos + 5];
                                uint8_t intf_subclass = full_config[pos + 6];
                                uint8_t intf_protocol = full_config[pos + 7];
                                
                                if (intf_class == 0x03) { // HID class interface
                                    device_info->is_hid = true;
                                    device_info->current_interface = full_config[pos + 2]; // Interface number
                                    break;
                                }
                            }
                            
                            // Move to next descriptor
                            pos += full_config[pos]; // bLength
                        }
                        
                        // If we found a HID interface, look for its endpoints
                        if (device_info->is_hid) {
                            pos = 9; // Reset to start of first interface
                            bool in_our_interface = false;
                            
                            while (pos < total_length) {
                                if (pos + 9 > total_length) break;
                                
                                // Check if this is an interface descriptor
                                if (full_config[pos + 1] == 0x04) {
                                    in_our_interface = (full_config[pos + 2] == device_info->current_interface);
                                }
                                // If in our interface and this is an endpoint descriptor
                                else if (in_our_interface && full_config[pos + 1] == 0x05) {
                                    uint8_t ep_addr = full_config[pos + 2];
                                    uint8_t ep_attr = full_config[pos + 3];
                                    
                                    // Check if this is an interrupt endpoint
                                    if ((ep_attr & 0x03) == 0x03) {
                                        if (ep_addr & 0x80) { // IN endpoint
                                            device_info->endpoint_in = ep_addr;
                                        } else { // OUT endpoint
                                            device_info->endpoint_out = ep_addr;
                                        }
                                    }
                                }
                                
                                // Move to next descriptor
                                pos += full_config[pos];
                            }
                        }
                    }
                }
                
                free(full_config);
            }
        }
    }
    
    return true;
}

/**
 * Configure a HID device for operation
 */
static bool configure_hid_device(usb_device_info_t* device_info)
{
    if (!device_info || !device_info->is_hid) {
        return false;
    }
    
    printf("[LPC55S69-Host Handler] Configuring HID device\n");
    
    // Set configuration 1 (standard for most USB devices)
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x00,  // Host-to-device, standard, device
        .bRequest = 0x09,       // SET_CONFIGURATION
        .wValue = 0x0001,       // Configuration 1
        .wIndex = 0,
        .wLength = 0
    };
    
    int result = hurricane_hw_host_control_transfer(&setup, NULL, 0);
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to set configuration, error %d\n", result);
        return false;
    }
    
    // For HID devices, set the protocol to report protocol (not boot protocol)
    setup.bmRequestType = 0x21;  // Host-to-device, class, interface
    setup.bRequest = USB_HID_SET_PROTOCOL;
    setup.wValue = 0x0001;       // Report protocol
    setup.wIndex = device_info->current_interface;
    setup.wLength = 0;
    
    result = hurricane_hw_host_control_transfer(&setup, NULL, 0);
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to set protocol, error %d (not fatal)\n", result);
        // Continue anyway, as some devices might not support this
    }
    
    // Set idle rate to reduce unnecessary reports for keyboards
    setup.bmRequestType = 0x21;  // Host-to-device, class, interface
    setup.bRequest = USB_HID_SET_IDLE;
    setup.wValue = 0x0000;       // Indefinite idle, report ID 0
    setup.wIndex = device_info->current_interface;
    setup.wLength = 0;
    
    result = hurricane_hw_host_control_transfer(&setup, NULL, 0);
    if (result < 0) {
        printf("[LPC55S69-Host Handler] Failed to set idle rate, error %d (not fatal)\n", result);
        // Continue anyway, as some devices might not support this
    }
    
    printf("[LPC55S69-Host Handler] HID device configured successfully\n");
    return true;
}

/**
 * Process a HID report received from a device
 */
static void process_hid_report(const uint8_t* report_data, uint16_t length, uint8_t endpoint)
{
    if (!report_data || length == 0) {
        return;
    }
    
    printf("[LPC55S69-Host Handler] Processing HID report (%d bytes)\n", length);
    
    // Store the report in our buffer
    last_report.report_id = (length > 0) ? 0 : 0; // No report ID in current implementation
    memcpy(last_report.data, report_data, (length > sizeof(last_report.data)) ? sizeof(last_report.data) : length);
    last_report.length = (length > sizeof(last_report.data)) ? sizeof(last_report.data) : length;
    last_report.timestamp = hurricane_get_time_ms();
    new_report_available = true;
    
    // If this is from a mouse, interpret the data
    if (active_device_idx >= 0 && 
        devices[active_device_idx].is_hid && 
        devices[active_device_idx].device_protocol == 2) { // Mouse protocol
        
        // Typical mouse report format: [button_mask, x_delta, y_delta]
        if (length >= 3) {
            uint8_t buttons = report_data[0];
            int8_t dx = (int8_t)report_data[1];
            int8_t dy = (int8_t)report_data[2];
            
            printf("[LPC55S69-Host Handler] Mouse report: buttons=%02X, dx=%d, dy=%d\n", 
                   buttons, dx, dy);
        }
    }
    // If this is from a keyboard, interpret the data
    else if (active_device_idx >= 0 && 
             devices[active_device_idx].is_hid && 
             devices[active_device_idx].device_protocol == 1) { // Keyboard protocol
        
        // Typical keyboard report format: [modifier, reserved, keycode1, keycode2, ...]
        if (length >= 3) {
            uint8_t modifier = report_data[0];
            uint8_t keycode1 = report_data[2];
            
            printf("[LPC55S69-Host Handler] Keyboard report: modifier=%02X, key1=%02X\n", 
                   modifier, keycode1);
        }
    }
    
    // Call the registered callback if there is one
    if (report_callback) {
        report_callback(&last_report);
    }
}

/**
 * Print device information to console
 */
static void print_device_info(const usb_device_info_t* device_info)
{
    if (!device_info) {
        return;
    }
    
    printf("\n[LPC55S69-Host Handler] USB Device Information:\n");
    printf("  VID:PID      : %04X:%04X\n", device_info->vendor_id, device_info->product_id);
    printf("  Product      : %s\n", device_info->product_name);
    printf("  Device Class : %02X (Subclass: %02X, Protocol: %02X)\n", 
           device_info->device_class, device_info->device_subclass, device_info->device_protocol);
    printf("  HID Device   : %s\n", device_info->is_hid ? "Yes" : "No");
    if (device_info->is_hid) {
        printf("  Interface    : %d\n", device_info->current_interface);
        printf("  Endpoints    : IN=0x%02X, OUT=0x%02X\n", 
               device_info->endpoint_in, device_info->endpoint_out);
    }
    printf("  Interfaces   : %d\n", device_info->interface_count);
    printf("\n");
}