/**
 * @file host_handler.h
 * @brief Host mode handler for dual HID example
 *
 * This file provides the necessary definitions and functions for the host mode
 * of the dual HID example, including device detection, HID device handling,
 * and data reception from connected devices.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hurricane.h"
#include "core/usb_interface_manager.h"

/**
 * @brief USB device information structure
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    bool is_hid;
    uint8_t interface_count;
    uint8_t current_interface;
    uint8_t endpoint_in;
    uint8_t endpoint_out;
    bool connected;
    char product_name[32];
} usb_device_info_t;

/**
 * @brief HID report data structure
 */
typedef struct {
    uint8_t report_id;
    uint8_t data[64];
    uint16_t length;
    uint32_t timestamp;
} hid_report_data_t;

/**
 * @brief Initialize the host-mode handler
 */
void host_handler_init(void);

/**
 * @brief Deinitialize the host-mode handler
 */
void host_handler_deinit(void);

/**
 * @brief Run host-mode tasks
 * 
 * Poll for device attachments, process incoming data, etc.
 */
void host_handler_task(void);

/**
 * @brief Check if a device is attached to the host controller
 * 
 * @return true if a device is attached, false otherwise
 */
bool host_handler_is_device_connected(void);

/**
 * @brief Get information about the currently connected device
 * 
 * @param device_info Pointer to structure to fill with device information
 * @return true if device info was filled, false if no device is connected
 */
bool host_handler_get_device_info(usb_device_info_t* device_info);

/**
 * @brief Read a HID report from the connected device
 * 
 * Reads the latest HID report from the connected device if available
 * 
 * @param report Pointer to structure to fill with report data
 * @return true if a report was read, false if no report is available
 */
bool host_handler_read_hid_report(hid_report_data_t* report);

/**
 * @brief Send a HID report to the connected device
 * 
 * @param report_id Report ID (0 if not using report IDs)
 * @param data Report data buffer
 * @param length Length of the report data
 * @return true if successful, false if failed
 */
bool host_handler_send_hid_report(uint8_t report_id, const uint8_t* data, uint16_t length);

/**
 * @brief Set the LED states for a HID keyboard
 * 
 * @param leds LED states (bit 0 = NUM, bit 1 = CAPS, bit 2 = SCROLL)
 * @return true if successful, false if failed
 */
bool host_handler_set_keyboard_leds(uint8_t leds);

/**
 * @brief Register callback function for HID report reception
 * 
 * @param callback Function pointer to call when a HID report is received
 */
void host_handler_register_report_callback(void (*callback)(const hid_report_data_t* report));

/**
 * @brief Get the HID report descriptor from the connected device
 * 
 * @param descriptor Buffer to store the descriptor
 * @param max_length Maximum buffer length
 * @return Actual descriptor length, or 0 if failed
 */
uint16_t host_handler_get_report_descriptor(uint8_t* descriptor, uint16_t max_length);