#pragma once

#include <stdint.h>
#include "usb_descriptor.h"
#include "hurricane_hw_hal.h"

/*! @brief Defines USB 2.0 device state */
typedef enum _usb_device_state
{
    kUSB_DeviceStateConfigured = 0U, /*!< Device state, Configured*/
    kUSB_DeviceStateAddress,         /*!< Device state, Address*/
    kUSB_DeviceStateDefault,         /*!< Device state, Default*/
    kUSB_DeviceStateAddressing,      /*!< Device state, Address setting*/
    kUSB_DeviceStateTestMode,        /*!< Device state, Test mode*/
} usb_device_state_t;

typedef struct {
    usb_device_state_t state;
    uint8_t device_address;
    usb_device_descriptor_t device_desc; // Store parsed descriptor
    
    // HID device tracking
    uint8_t hid_configured;    // Flag to indicate HID device is configured
    uint8_t hid_interface;     // Interface number for HID device
    uint8_t hid_endpoint;      // Endpoint address for HID interrupt IN
} usb_device_t;

void usb_host_init(void);
void usb_host_poll(void);
