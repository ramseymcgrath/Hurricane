// File: lib/hurricane/hw/hurricane_hw_hal.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Hurricane Dual USB Stack Version
 */
#define HURRICANE_USB_VERSION_MAJOR 2
#define HURRICANE_USB_VERSION_MINOR 0
#define HURRICANE_USB_VERSION_PATCH 0

// USB setup packet structure
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} hurricane_usb_setup_packet_t;

//=============================================================================
// Common functions (work with both host and device stacks)
//=============================================================================

/**
 * @brief Initialize hardware for both USB host and device stacks
 * 
 * This function initializes both USB controllers and peripherals needed for
 * simultaneous host and device operation.
 */
void hurricane_hw_init(void);

/**
 * @brief Run per-loop USB polling or maintenance for both stacks
 * 
 * This function handles both host and device stack operations in each
 * iteration of the main loop.
 */
void hurricane_hw_poll(void);

/**
 * @brief Synchronize host and device controllers when needed
 *
 * This function ensures proper synchronization between host and device
 * controllers when they need to coordinate operations.
 */
void hurricane_hw_sync_controllers(void);

//=============================================================================
// Host mode functions
//=============================================================================

/**
 * @brief Initialize hardware for USB host operation
 * 
 * This function initializes the hardware USB host stack.
 * For backward compatibility, it may also be called by hurricane_hw_init().
 */
void hurricane_hw_host_init(void);

/**
 * @brief Run per-loop USB polling or maintenance for host controller
 * 
 * This function handles host stack operations in each iteration of the main loop.
 * For backward compatibility, it may also be called by hurricane_hw_poll().
 */
void hurricane_hw_host_poll(void);

/**
 * @brief Check if a device is attached to the host controller
 * 
 * @return Non-zero if device is connected, 0 otherwise
 */
int hurricane_hw_host_device_connected(void);

/**
 * @brief Reset the USB host bus (full detach + reattach)
 */
void hurricane_hw_host_reset_bus(void);

/**
 * @brief Perform a USB control transfer in host mode
 * 
 * @param setup Pointer to setup packet
 * @param buffer Data buffer (for data stage)
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_host_control_transfer(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length
);

/**
 * @brief Perform a USB interrupt IN transfer in host mode
 * 
 * @param endpoint Endpoint address 
 * @param buffer Data buffer
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_host_interrupt_in_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length
);

//=============================================================================
// Device mode functions
//=============================================================================

/**
 * @brief Initialize hardware for USB device operation
 * 
 * This function initializes the hardware USB device stack.
 */
void hurricane_hw_device_init(void);

/**
 * @brief Run per-loop USB polling or maintenance for device controller
 * 
 * This function handles device stack operations in each iteration of the main loop.
 */
void hurricane_hw_device_poll(void);

/**
 * @brief Check if host is connected to device controller
 * 
 * @return Non-zero if host is connected, 0 otherwise
 */
int hurricane_hw_device_host_connected(void);

/**
 * @brief Reset the USB device controller
 * 
 * This triggers a disconnect/connect cycle to notify host of device changes.
 */
void hurricane_hw_device_reset(void);

/**
 * @brief Respond to a control transfer in device mode
 * 
 * @param setup Pointer to setup packet received
 * @param buffer Data buffer (for data stage)
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_device_control_response(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length
);

/**
 * @brief Perform a USB interrupt IN transfer in device mode (send data to host)
 * 
 * @param endpoint Endpoint address 
 * @param buffer Data buffer
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_device_interrupt_in_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length
);

/**
 * @brief Perform a USB interrupt OUT transfer in device mode (receive data from host)
 * 
 * @param endpoint Endpoint address 
 * @param buffer Data buffer
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_device_interrupt_out_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length
);

/**
 * @brief Set callback for device set configuration event
 * 
 * @param callback Function to call when SET_CONFIGURATION request is received
 */
void hurricane_hw_device_set_configuration_callback(
    void (*callback)(uint8_t configuration)
);

/**
 * @brief Set callback for device set interface event
 * 
 * @param callback Function to call when SET_INTERFACE request is received
 */
void hurricane_hw_device_set_interface_callback(
    void (*callback)(uint8_t interface, uint8_t alt_setting)
);

/**
 * @brief Configure a USB interface in device mode
 * 
 * @param interface_num Interface number
 * @param interface_class USB class code
 * @param interface_subclass USB subclass code
 * @param interface_protocol Protocol code
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_configure_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol
);

/**
 * @brief Configure a USB endpoint in device mode
 * 
 * @param interface_num Interface number the endpoint belongs to
 * @param ep_address Endpoint address including direction bit
 * @param ep_attributes Endpoint attributes (type, etc.)
 * @param ep_max_packet_size Maximum packet size
 * @param ep_interval Polling interval
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval
);

/**
 * @brief Set device descriptors
 * 
 * @param device_desc Pointer to device descriptor
 * @param device_desc_length Length of device descriptor
 * @param config_desc Pointer to configuration descriptor
 * @param config_desc_length Length of configuration descriptor
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_set_descriptors(
    const uint8_t* device_desc,
    uint16_t device_desc_length,
    const uint8_t* config_desc,
    uint16_t config_desc_length
);

/**
 * @brief Set HID report descriptor
 * 
 * @param report_desc Pointer to HID report descriptor
 * @param report_desc_length Length of HID report descriptor
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_set_hid_report_descriptor(
    const uint8_t* report_desc,
    uint16_t report_desc_length
);

//=============================================================================
// Additional functions for dynamic interface configuration
//=============================================================================

/**
 * @brief Set string descriptor for device mode
 *
 * @param index String descriptor index
 * @param str_desc Pointer to string descriptor
 * @param str_desc_length Length of string descriptor
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_set_string_descriptor(
    uint8_t index,
    const uint8_t* str_desc,
    uint16_t str_desc_length
);

/**
 * @brief Enable or disable a configured endpoint
 *
 * @param ep_address Endpoint address
 * @param enable True to enable endpoint, false to disable
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_endpoint_enable(
    uint8_t ep_address,
    bool enable
);

/**
 * @brief Stall or unstall an endpoint
 *
 * @param ep_address Endpoint address
 * @param stall True to stall endpoint, false to unstall
 * @return 0 on success, negative error code on failure
 */
int hurricane_hw_device_endpoint_stall(
    uint8_t ep_address,
    bool stall
);

//=============================================================================
// Backward compatibility functions
//=============================================================================

// For backward compatibility with existing code
#define hurricane_hw_device_connected hurricane_hw_host_device_connected
#define hurricane_hw_reset_bus hurricane_hw_host_reset_bus
#define hurricane_hw_control_transfer hurricane_hw_host_control_transfer
#define hurricane_hw_interrupt_in_transfer hurricane_hw_host_interrupt_in_transfer
