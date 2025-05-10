#pragma once

#include <stdint.h>
#include "usb_descriptor.h"
#include "hurricane_hw_hal.h"

/**
 * @brief Represents the state of a USB device connected to the host.
 *
 * This enum is used by the host controller to track the enumeration
 * and operational state of an attached USB device.
 */
typedef enum _hurricane_host_device_state // Renamed from _usb_device_state
{
    kHurricane_Host_DeviceStateConfigured = 0U, /*!< Device state, Configured*/
    kHurricane_Host_DeviceStateAddress,         /*!< Device state, Address*/
    kHurricane_Host_DeviceStateDefault,         /*!< Device state, Default*/
    kHurricane_Host_DeviceStateAddressing,      /*!< Device state, Address setting*/
    kHurricane_Host_DeviceStateTestMode,        /*!< Device state, Test mode*/
    // Add any other host-specific states if needed, e.g.:
    // kHurricane_Host_DeviceStateDetached,
    // kHurricane_Host_DeviceStateEnumerating,
    // kHurricane_Host_DeviceStateError
} hurricane_host_device_state_t; // Renamed typedef

/**
 * @brief Structure to hold information about a connected USB device.
 *
 * This structure is used by the host controller to maintain
 * information about the current state and configuration of
 * a USB device that is connected to the host.
 */
typedef struct {
    hurricane_host_device_state_t state; /*!< Current state of the USB device */
    uint8_t device_address;              /*!< Device address assigned by the host */
    usb_device_descriptor_t device_desc; // Store parsed descriptor
    
    // HID device tracking
    uint8_t hid_configured;    // Flag to indicate HID device is configured
    uint8_t hid_interface;     // Interface number for HID device
    uint8_t hid_endpoint;      // Endpoint address for HID interrupt IN
} usb_device_t;

void usb_host_init(void);
void usb_host_poll(void);
