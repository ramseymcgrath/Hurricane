/**
 * @file device_config.h
 * @brief Device mode configuration for dual HID example
 *
 * This file provides the necessary configuration for the device mode of
 * the dual HID example, including descriptors, endpoints, and report generation
 * for a composite HID device (mouse + keyboard).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hurricane.h"
#include "core/usb_interface_manager.h"

/**
 * @brief HID interface configuration data
 */
typedef struct {
    uint8_t interface_num;
    const uint8_t* report_descriptor;
    uint16_t report_descriptor_length;
    uint8_t in_endpoint;
    uint8_t protocol;  // 1 = Keyboard, 2 = Mouse, 0 = Other
    bool configured;
} hid_config_t;

/**
 * @brief Initialize the device-mode configuration
 * 
 * Sets up descriptors, interfaces, and endpoints for the device mode
 */
void device_config_init(void);

/**
 * @brief Deinitialize device-mode configuration
 */
void device_config_deinit(void);

/**
 * @brief Run device-mode tasks
 * 
 * Generates and sends HID reports based on timing or events
 */
void device_config_task(void);

/**
 * @brief Handle USB device configuration changes
 * 
 * @param configuration The new configuration value
 */
void device_config_set_configuration_callback(uint8_t configuration);

/**
 * @brief Handle USB interface setting changes
 * 
 * @param interface Interface that changed
 * @param alt_setting New alternate setting
 */
void device_config_set_interface_callback(uint8_t interface, uint8_t alt_setting);

/**
 * @brief Check if device has an active connection to a host
 * 
 * @return true if connected, false otherwise
 */
bool device_config_is_connected(void);

/**
 * @brief Generate and send a mouse movement report
 * 
 * @param dx X movement (-127 to 127)
 * @param dy Y movement (-127 to 127)
 * @param buttons Button state (bit 0 = left, bit 1 = right, bit 2 = middle)
 * @return int 0 if successful, error code otherwise
 */
int device_config_send_mouse_report(int8_t dx, int8_t dy, uint8_t buttons);

/**
 * @brief Generate and send a keyboard report
 * 
 * @param modifier Modifier keys (CTRL, SHIFT, ALT, etc)
 * @param keycodes Array of up to 6 keycodes (0 = no key)
 * @return int 0 if successful, error code otherwise
 */
int device_config_send_keyboard_report(uint8_t modifier, const uint8_t keycodes[6]);

/**
 * @brief Get the current mouse HID configuration
 * 
 * @return const hid_config_t* Pointer to mouse HID configuration
 */
const hid_config_t* device_config_get_mouse_config(void);

/**
 * @brief Get the current keyboard HID configuration
 * 
 * @return const hid_config_t* Pointer to keyboard HID configuration
 */
const hid_config_t* device_config_get_keyboard_config(void);