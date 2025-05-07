/**
 * @file usb_hw_hal_host_lpc55s69.c
 * @brief LPC55S69-specific USB host HAL implementation
 * 
 * This file implements the host-mode USB HAL functions for the LPC55S69 platform
 * using the NXP SDK. It uses the USB1 controller in host mode (High Speed EHCI).
 */

#include "hurricane_hw_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// NXP SDK includes
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "fsl_power.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "usb_host_config.h"
#include "usb.h"
#include "usb_host.h"
#include "usb_host_hid.h"

//==============================================================================
// Private definitions and variables
//==============================================================================

static usb_host_handle host_handle;

static bool host_initialized = false;
static bool device_connected = false;
static bool device_enumerated = false;
static uint8_t device_address = 0;

typedef enum {
    ENUM_STATE_IDLE,
    ENUM_STATE_GET_DEVICE_DESC,
    ENUM_STATE_GET_FULL_DEVICE_DESC,
    ENUM_STATE_GET_CONFIG_DESC,
    ENUM_STATE_GET_FULL_CONFIG_DESC,
    ENUM_STATE_SET_ADDRESS,
    ENUM_STATE_SET_CONFIGURATION,
    ENUM_STATE_COMPLETE
} enum_state_t;

static enum_state_t enum_state = ENUM_STATE_IDLE;
static uint8_t enum_retries = 0;
static uint16_t enum_config_total_length = 0;

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t max_packet_size;
    uint8_t num_configurations;
    uint8_t current_config;
    uint8_t interface_count;
    uint8_t descriptor_buffer[512];
} device_info_t;

static device_info_t device_info;

#define TRANSFER_BUFFER_SIZE 1024
static uint8_t transfer_buffer[TRANSFER_BUFFER_SIZE];

static void USB_HostCallback(usb_host_handle handle, uint32_t event, void *param);
static void USB_HostHidCallback(void* param, uint8_t* buffer, uint32_t length);
static void process_enumeration_state(void);
static usb_status_t get_device_descriptor(bool full);
static usb_status_t get_config_descriptor(bool full);
static usb_status_t set_device_address(uint8_t address);
static usb_status_t set_device_configuration(uint8_t config);
static void parse_device_descriptor(void);
static void parse_config_descriptor(void);
static void handle_enumeration_error(usb_status_t status);

extern void usb_host_hw_init(void);
extern void USB_HostIsrEnable(void);

//==============================================================================
// Public HAL functions
//==============================================================================

bool hurricane_hw_host_device_connected(void)
{
    /* Check USB1_PORTSC1 register's CCS (Current Connect Status) bit */
    return (USB1->PORTSC1 & USB_PORTSC1_CCS_MASK) != 0;
}

void hurricane_hw_host_poll(void)
{
    if(host_initialized) {
        USB_HostTaskFn(host_handle);
        process_enumeration_state();
    }
}

hurricane_status_t hurricane_hw_host_control_transfer(uint8_t bmRequestType,
                                                     uint8_t bRequest,
                                                     uint16_t wValue,
                                                     uint16_t wIndex,
                                                     uint16_t wLength,
                                                     uint8_t* data)
{
    usb_status_t status;
    usb_host_pipe_handle pipe_handle;
    usb_host_transfer_t* transfer;
    
    if(!host_initialized || !device_enumerated) {
        return HURRICANE_ERR_NOT_READY;
    }

    status = USB_HostControlTransfer(host_handle,
                                   bmRequestType,
                                   bRequest,
                                   wValue,
                                   wIndex,
                                   wLength,
                                   data,
                                   USB_HOST_TO_DEVICE,
                                   &pipe_handle,
                                   &transfer);
    
    if(status != kStatus_USB_Success) {
        return HURRICANE_ERR_TRANSFER;
    }

    while(!(transfer->transferStatus & USB_HOST_TRANSFER_STATUS_COMPLETE)) {
        hurricane_hw_host_poll();
    }

    return transfer->transferStatus == USB_HOST_TRANSFER_STATUS_COMPLETE ?
        HURRICANE_OK : HURRICANE_ERR_TRANSFER;
}
