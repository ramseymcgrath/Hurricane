/**
 * @file main.c
 * @brief Dual HID example application using Hurricane USB Interface Manager for LPC55S69
 *
 * This example demonstrates the use of the Hurricane USB Interface Manager
 * to simultaneously operate a USB host controller with HID device support
 * and a USB device controller presenting a composite HID device (mouse + keyboard).
 * 
 * The application shows proper coordination between host and device modes,
 * dynamic interface configuration, and structured component organization.
 * 
 * For LPC55S69, the physically separate controllers simplify the implementation:
 * - USB0 is used in device mode (Full Speed)
 * - USB1 is used in host mode (High Speed EHCI)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hurricane.h"
#include "core/usb_interface_manager.h"
#include "device_config.h"
#include "host_handler.h"

// Application state
typedef enum {
    APP_STATE_INIT,
    APP_STATE_RUNNING,
    APP_STATE_ERROR,
    APP_STATE_SHUTDOWN
} app_state_t;

static app_state_t app_state = APP_STATE_INIT;

// Callback function for HID reports from host mode
static void hid_report_callback(const hid_report_data_t* report);

// Callback function for keyboard LED state changes from device mode
static void keyboard_led_callback(uint8_t led_state);

// Callback function type for keyboard LED state changes
typedef void (*keyboard_led_callback_t)(uint8_t);

// Global callback variable - used by device_config.c
keyboard_led_callback_t g_keyboard_led_callback = NULL;

// Configuration callbacks
static void set_configuration_callback(uint8_t configuration);
static void set_interface_callback(uint8_t interface, uint8_t alt_setting);

/**
 * @brief Initialize the dual USB application
 * 
 * Initializes both USB controllers and related components
 * 
 * @return true if initialization succeeded, false otherwise
 */
static bool app_init(void)
{
    printf("\n==================================================\n");
    printf("==================================================\n\n");

    // Initialize Hurricane core
    hurricane_init();
    
    // Initialize USB Interface Manager
    hurricane_interface_manager_init();
    
    // Initialize both host and device USB stacks
    hurricane_usb_init();
    
    // Register configuration/interface callbacks
    hurricane_hw_device_set_configuration_callback(set_configuration_callback);
    hurricane_hw_device_set_interface_callback(set_interface_callback);
    
    // Initialize device mode configuration
    printf("\n[Main] Setting up USB device mode (USB0)...\n");
    device_config_init();
    
    // Initialize host mode handler
    printf("\n[Main] Setting up USB host mode (USB1)...\n");
    host_handler_init();
    
    // Register callback for host HID report reception
    host_handler_register_report_callback(hid_report_callback);
    
    // Register callback for keyboard LED state changes
    g_keyboard_led_callback = keyboard_led_callback;
    
    printf("\n[Main] Dual USB initialization complete\n");
    printf("* Device mode (USB0): Composite HID (Mouse + Keyboard)\n");
    printf("* Host mode (USB1): HID device detection and handling\n\n");
    
    return true;
}

/**
 * @brief Run a single iteration of the application loop
 * 
 * This function processes both host and device mode tasks,
 * handles events, and maintains system operation
 */
static void app_run_iteration(void)
{
    // Run Hurricane USB tasks (both host and device controllers)
    hurricane_task();
    
    // Run device-specific tasks (handle device mode)
    device_config_task();
    
    // Run host-specific tasks (handle host mode)
    host_handler_task();
    
    // Status information updates
    static uint32_t last_status_time = 0;
    uint32_t current_time = hurricane_get_time_ms();
    
    if (current_time - last_status_time > 5000) {
        printf("\n[Main] Status Update - Time: %lu ms\n", current_time);
        
        // Device connection status
        printf("  Device Mode (USB0): %s\n", 
            device_config_is_connected() ? "Connected to Host" : "Not Connected");
        
        // Host connection status
        usb_device_info_t device_info;
        if (host_handler_is_device_connected() && host_handler_get_device_info(&device_info)) {
            printf("  Host Mode (USB1): Connected to %s (VID: %04X, PID: %04X)\n", 
                device_info.product_name[0] ? device_info.product_name : "USB Device",
                device_info.vendor_id, device_info.product_id);
        } else {
            printf("  Host Mode (USB1): No Device Connected\n");
        }
        
        printf("\n");
        last_status_time = current_time;
    }
    
    // Brief delay to reduce CPU usage
    hurricane_delay_ms(1);
}

/**
 * @brief Clean up and shutdown the application
 */
static void app_shutdown(void)
{
    printf("\n[Main] Shutting down dual USB application\n");
    
    // Deinitialize host handler
    host_handler_deinit();
    
    // Deinitialize device configuration
    device_config_deinit();
    
    // Deinitialize the Interface Manager
    hurricane_interface_manager_deinit();
    
    printf("[Main] Shutdown complete\n");
}

/**
 * Application entry point
 */
int main(void)
{
    // Initialize the application
    if (!app_init()) {
        printf("[Main] Initialization failed!\n");
        return -1;
    }
    
    app_state = APP_STATE_RUNNING;
    
    // Main application loop
    printf("[Main] Entering main loop\n\n");
    while (app_state == APP_STATE_RUNNING) {
        app_run_iteration();
    }
    
    // Cleanup
    app_shutdown();
    
    return app_state == APP_STATE_ERROR ? -1 : 0;
}

/**
 * Device configuration change callback
 */
static void set_configuration_callback(uint8_t configuration)
{
    printf("[Main] Device configuration changed to %d\n", configuration);
    device_config_set_configuration_callback(configuration);
}

/**
 * Device interface setting change callback
 */
static void set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
    printf("[Main] Interface %d alternate setting changed to %d\n", 
           interface, alt_setting);
    device_config_set_interface_callback(interface, alt_setting);
}

/**
 * Callback for keyboard LED state changes from device mode
 *
 * This handles bidirectional communication by forwarding LED state
 * changes from the host connected to our device mode (USB0)
 * to the keyboard connected to our host mode (USB1)
 */
static void keyboard_led_callback(uint8_t led_state)
{
    printf("[Main] Forwarding keyboard LED state: 0x%02X to physical keyboard\n", led_state);
    
    // Only forward if we have a device connected to the host controller
    if (host_handler_is_device_connected()) {
        // Get device info to make sure it's a keyboard
        usb_device_info_t device_info;
        if (host_handler_get_device_info(&device_info)) {
            if (device_info.device_protocol == 1 || device_info.is_hid) { // Keyboard or HID device
                // Send LED state to the physical keyboard
                if (!host_handler_set_keyboard_leds(led_state)) {
                    printf("[Main] Failed to forward LED state to physical keyboard\n");
                }
            }
        }
    }
}

/**
 * Callback for HID reports received in host mode
 */
static void hid_report_callback(const hid_report_data_t* report)
{
    if (!report) return;
    
    // Display basic report information
    printf("[Main] Received HID report: ID: %d, Length: %d\n",
           report->report_id, report->length);
    
    // Check if we're connected to the device controller
    if (!device_config_is_connected()) {
        printf("[Main] Device controller not connected to a host, not relaying report\n");
        return;
    }
    
    // We need device information to determine report type
    usb_device_info_t device_info;
    if (!host_handler_get_device_info(&device_info)) {
        printf("[Main] Failed to get device info, cannot relay HID report\n");
        return;
    }
    
    // Process based on device protocol or report size/format
    if (device_info.device_protocol == 2 || // Mouse protocol
        (report->length >= 3 && report->length <= 4)) { // Typical mouse report length
        
        // Parse mouse report data - typically [buttons, x, y, wheel(optional)]
        uint8_t buttons = report->data[0];
        int8_t dx = (int8_t)report->data[1];
        int8_t dy = (int8_t)report->data[2];
        
        printf("[Main] Relaying mouse report: buttons=0x%02X, dx=%d, dy=%d\n",
               buttons, dx, dy);
        
        // Send report to device controller (USB0)
        int result = device_config_send_mouse_report(dx, dy, buttons);
        if (result != 0) {
            printf("[Main] Failed to relay mouse report, error %d\n", result);
        }
    }
    else if (device_info.device_protocol == 1 || // Keyboard protocol
             (report->length >= 8)) { // Typical keyboard report length
        
        // Parse keyboard report data - typically [modifier, reserved, key1, key2, key3, key4, key5, key6]
        uint8_t modifier = report->data[0];
        uint8_t keycodes[6]; // Maximum 6 keys in standard HID keyboard report
        
        // Copy key codes, starting from byte 2 (skipping modifier and reserved bytes)
        // Ensure we don't exceed report length or the 6-key limit
        size_t key_bytes = report->length - 2 > 6 ? 6 : report->length - 2;
        if (key_bytes > 0) {
            memcpy(keycodes, &report->data[2], key_bytes);
        }
        
        // Zero out any remaining key slots
        for (size_t i = key_bytes; i < 6; i++) {
            keycodes[i] = 0;
        }
        
        printf("[Main] Relaying keyboard report: modifier=0x%02X, keys=[0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X]\n",
               modifier, keycodes[0], keycodes[1], keycodes[2], keycodes[3], keycodes[4], keycodes[5]);
        
        // Send report to device controller (USB0)
        int result = device_config_send_keyboard_report(modifier, keycodes);
        if (result != 0) {
            printf("[Main] Failed to relay keyboard report, error %d\n", result);
        }
        
        // For bidirectional communication, monitor changes in LED state
        // This would be handled by the device controller callbacks
        // The host_handler_set_keyboard_leds() function would be called
        // when the host (connected to USB0) changes the LED state
    }
    else {
        printf("[Main] Unknown HID report format, cannot relay\n");
    }
}