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
    printf("Hurricane Dual USB Stack - LPC55S69 HID Example\n");
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
 * Callback for HID reports received in host mode
 */
static void hid_report_callback(const hid_report_data_t* report)
{
    if (!report) return;
    
    // Display basic report information
    printf("[Main] Received HID report: ID: %d, Length: %d\n", 
           report->report_id, report->length);
    
    // For keyboard reports, we might want to change keyboard LEDs based on state
    // This demonstrates bidirectional communication with a HID device
    
    // For mouse reports, we could process the movement data
    
    // Example: relay certain HID reports to the device mode
    // This would allow creating a HID proxy where data from a physical
    // device is transformed and sent to the host connected to our device controller
}