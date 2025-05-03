/**
 * @file usb_hw_hal_host_lpc55s69.c
 * @brief LPC55S69-specific USB host HAL implementation
 * 
 * This file implements the host-mode USB HAL functions for the LPC55S69 platform
 * using the NXP SDK. It uses the USB1 controller in host mode (High Speed EHCI).
 */

#include "hw/hurricane_hw_hal.h"
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
#include "usb_phy.h"

//==============================================================================
// Private definitions and variables
//==============================================================================

// USB host controller handle
static usb_host_handle host_handle;

// Host stack status
static bool host_initialized = false;
static bool device_connected = false;
static bool device_enumerated = false;
static uint8_t device_address = 0;

// Enumeration state machine
typedef enum {
    ENUM_STATE_IDLE,                  // No enumeration in progress
    ENUM_STATE_GET_DEVICE_DESC,       // Getting device descriptor (short)
    ENUM_STATE_GET_FULL_DEVICE_DESC,  // Getting full device descriptor
    ENUM_STATE_GET_CONFIG_DESC,       // Getting configuration descriptor (header)
    ENUM_STATE_GET_FULL_CONFIG_DESC,  // Getting full configuration descriptor
    ENUM_STATE_SET_ADDRESS,           // Setting device address
    ENUM_STATE_SET_CONFIGURATION,     // Setting device configuration
    ENUM_STATE_COMPLETE               // Enumeration completed
} enum_state_t;

// Enumeration tracking
static enum_state_t enum_state = ENUM_STATE_IDLE;
static uint8_t enum_retries = 0;
static uint16_t enum_config_total_length = 0;

// Device information
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
    // Buffer to store descriptors during enumeration
    uint8_t descriptor_buffer[512];
} device_info_t;

static device_info_t device_info;

// Transfer buffer for control and interrupt transfers
#define TRANSFER_BUFFER_SIZE 1024
static uint8_t transfer_buffer[TRANSFER_BUFFER_SIZE];

// Forward declarations
static void USB_HostCallback(usb_host_handle handle,
                            uint32_t event,
                            void *param);
static void USB_HostHidCallback(void* param,
                               uint8_t* buffer,
                               uint32_t length);
static void process_enumeration_state(void);
static usb_status_t get_device_descriptor(bool full);
static usb_status_t get_config_descriptor(bool full);
static usb_status_t set_device_address(uint8_t address);
static usb_status_t set_device_configuration(uint8_t config);
static void parse_device_descriptor(void);
static void parse_config_descriptor(void);
static void handle_enumeration_error(usb_status_t status);

// External declarations from initialization file
extern void usb_host_hw_init(void);
extern void USB_HostIsrEnable(void);

//==============================================================================
// Public HAL functions
//==============================================================================

void hurricane_hw_host_init(void)
{
    if (host_initialized) {
        printf("[LPC55S69-Host] Host already initialized\n");
        return;
    }

    usb_status_t status = kStatus_USB_Success;

    // Initialize USB1 hardware (High-Speed EHCI)
    usb_host_hw_init();

    // Initialize host controller
    status = USB_HostInit(kUSB_ControllerEhci1, &host_handle, USB_HostCallback);
    if (kStatus_USB_Success != status) {
        printf("[LPC55S69-Host] USB host controller initialization failed\n");
        return;
    }

    // Enable IRQ
    USB_HostIsrEnable();

    host_initialized = true;
    device_connected = false;
    device_enumerated = false;
    enum_state = ENUM_STATE_IDLE;
    
    // Clear device info
    memset(&device_info, 0, sizeof(device_info));
    
    printf("[LPC55S69-Host] USB host initialized (High-Speed)\n");
}

void hurricane_hw_host_poll(void)
{
    if (!host_initialized) {
        return;
    }
    
    // Call USB host tasks
    USB_HostTaskFn(host_handle);
    
    // Handle enumeration state machine if active
    if (enum_state != ENUM_STATE_IDLE && enum_state != ENUM_STATE_COMPLETE) {
        process_enumeration_state();
    }
}

int hurricane_hw_host_device_connected(void)
{
    return (device_connected && device_enumerated) ? 1 : 0;
}

void hurricane_hw_host_reset_bus(void)
{
    if (!host_initialized) {
        printf("[LPC55S69-Host] Host not initialized\n");
        return;
    }
    
    usb_status_t status = USB_HostResetDevice(host_handle, device_address);
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Host] Failed to reset USB bus: %d\n", status);
    } else {
        printf("[LPC55S69-Host] USB bus reset initiated\n");
    }
}

int hurricane_hw_host_control_transfer(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length)
{
    if (!host_initialized || !device_connected || !device_enumerated) {
        printf("[LPC55S69-Host] Host not initialized or device not connected\n");
        return -1;
    }
    
    if (!setup) {
        printf("[LPC55S69-Host] Invalid setup packet\n");
        return -1;
    }
    
    usb_host_transfer_t* transfer;
    
    // Allocate transfer
    if (USB_HostMallocTransfer(host_handle, &transfer) != kStatus_USB_Success) {
        printf("[LPC55S69-Host] Failed to allocate transfer\n");
        return -1;
    }
    
    // Setup the transfer
    transfer->transferBuffer = buffer ? buffer : transfer_buffer;
    transfer->transferLength = length;
    transfer->callbackFn = USB_HostHidCallback;
    transfer->callbackParam = NULL;
    
    // Create setup packet
    usb_host_transfer_setup_t* setup_packet = (usb_host_transfer_setup_t*)transfer->setupPacket;
    setup_packet->bmRequestType = setup->bmRequestType;
    setup_packet->bRequest = setup->bRequest;
    setup_packet->wValue = USB_SHORT_TO_LITTLE_ENDIAN(setup->wValue);
    setup_packet->wIndex = USB_SHORT_TO_LITTLE_ENDIAN(setup->wIndex);
    setup_packet->wLength = USB_SHORT_TO_LITTLE_ENDIAN(setup->wLength);
    
    // Send the control request
    usb_status_t status = USB_HostSendControlRequest(host_handle, device_address, transfer);
    
    if (status != kStatus_USB_Success) {
        USB_HostFreeTransfer(host_handle, transfer);
        printf("[LPC55S69-Host] Failed to send control request: %d\n", status);
        return -1;
    }
    
    // Wait for transfer completion by blocking TODO: should be asynchronous
    volatile bool transfer_complete = false;
    uint32_t timeout = 5000;  // 5 seconds timeout
    
    while (!transfer_complete && timeout > 0) {
        // Poll USB host tasks
        USB_HostTaskFn(host_handle);
        
        // Check if transfer is complete
        if (transfer->transferSofar > 0) {
            transfer_complete = true;
        }
        
        // Delay
        for (volatile int i = 0; i < 10000; i++) {
            __asm("nop");
        }
        
        timeout--;
    }
    
    if (!transfer_complete) {
        printf("[LPC55S69-Host] Control transfer timed out\n");
        USB_HostCancelTransfer(host_handle, device_address, transfer);
        return -1;
    }
    
    // Return the number of bytes transferred
    return transfer->transferSofar;
}

int hurricane_hw_host_interrupt_in_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length)
{
    if (!host_initialized || !device_connected || !device_enumerated) {
        printf("[LPC55S69-Host] Host not initialized or device not connected\n");
        return -1;
    }
    
    // Make sure this is an IN endpoint
    if (!(endpoint & 0x80)) {
        printf("[LPC55S69-Host] Invalid IN endpoint 0x%02x\n", endpoint);
        return -1;
    }
    
    usb_host_transfer_t* transfer;
    
    // Allocate transfer
    if (USB_HostMallocTransfer(host_handle, &transfer) != kStatus_USB_Success) {
        printf("[LPC55S69-Host] Failed to allocate transfer\n");
        return -1;
    }
    
    // Setup the transfer
    transfer->transferBuffer = buffer ? buffer : transfer_buffer;
    transfer->transferLength = length;
    transfer->callbackFn = USB_HostHidCallback;
    transfer->callbackParam = NULL;
    
    // Send the interrupt request
    usb_status_t status = USB_HostSendSetup(
        host_handle,
        device_address,
        endpoint & 0x0F,  // Endpoint number
        USB_ENDPOINT_INTERRUPT,
        USB_IN,
        length,
        transfer
    );
    
    if (status != kStatus_USB_Success) {
        USB_HostFreeTransfer(host_handle, transfer);
        printf("[LPC55S69-Host] Failed to send interrupt request: %d\n", status);
        return -1;
    }
    
    // Wait for transfer completion by blocking TODO: should be asynchronous
    volatile bool transfer_complete = false;
    uint32_t timeout = 1000;  // 1 second timeout
    
    while (!transfer_complete && timeout > 0) {
        // Poll USB host tasks
        USB_HostTaskFn(host_handle);
        
        // Check if transfer is complete
        if (transfer->transferSofar > 0) {
            transfer_complete = true;
        }
        
        // Delay
        for (volatile int i = 0; i < 10000; i++) {
            __asm("nop");
        }
        
        timeout--;
    }
    
    if (!transfer_complete) {
        printf("[LPC55S69-Host] Interrupt transfer timed out\n");
        USB_HostCancelTransfer(host_handle, device_address, transfer);
        return -1;
    }
    
    // Return the number of bytes transferred
    return transfer->transferSofar;
}

/**
 * @brief Perform a USB interrupt OUT transfer in host mode
 *
 * @param endpoint Endpoint address
 * @param buffer Data buffer
 * @param length Length of data buffer
 * @return Number of bytes transferred, or negative error code
 */
int hurricane_hw_host_interrupt_out_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length)
{
    if (!host_initialized || !device_connected || !device_enumerated) {
        printf("[LPC55S69-Host] Host not initialized or device not connected\n");
        return -1;
    }
    
    // Make sure this is an OUT endpoint
    if (endpoint & 0x80) {
        printf("[LPC55S69-Host] Invalid OUT endpoint 0x%02x\n", endpoint);
        return -1;
    }
    
    usb_host_transfer_t* transfer;
    
    // Allocate transfer
    if (USB_HostMallocTransfer(host_handle, &transfer) != kStatus_USB_Success) {
        printf("[LPC55S69-Host] Failed to allocate transfer\n");
        return -1;
    }
    
    // Setup the transfer
    transfer->transferBuffer = buffer ? buffer : transfer_buffer;
    transfer->transferLength = length;
    transfer->callbackFn = USB_HostHidCallback;
    transfer->callbackParam = NULL;
    
    // Send the interrupt request
    usb_status_t status = USB_HostSendSetup(
        host_handle,
        device_address,
        endpoint & 0x0F,  // Endpoint number
        USB_ENDPOINT_INTERRUPT,
        USB_OUT,
        length,
        transfer
    );
    
    if (status != kStatus_USB_Success) {
        USB_HostFreeTransfer(host_handle, transfer);
        printf("[LPC55S69-Host] Failed to send interrupt OUT request: %d\n", status);
        return -1;
    }
    
    // Wait for transfer completion by blocking TODO: should be asynchronous
    volatile bool transfer_complete = false;
    uint32_t timeout = 1000;  // 1 second timeout
    
    while (!transfer_complete && timeout > 0) {
        // Poll USB host tasks
        USB_HostTaskFn(host_handle);
        
        // Check if transfer is complete
        if (transfer->transferSofar > 0) {
            transfer_complete = true;
        }
        
        // Delay
        for (volatile int i = 0; i < 10000; i++) {
            __asm("nop");
        }
        
        timeout--;
    }
    
    if (!transfer_complete) {
        printf("[LPC55S69-Host] Interrupt OUT transfer timed out\n");
        USB_HostCancelTransfer(host_handle, device_address, transfer);
        return -1;
    }
    
    // Return the number of bytes transferred
    return transfer->transferSofar;
}

//==============================================================================
// Private functions
//==============================================================================

static void USB_HostCallback(usb_host_handle handle,
                            uint32_t event,
                            void *param)
{
    usb_status_t status = kStatus_USB_Success;
    
    switch (event) {
        case kUSB_HostEventAttach:
            device_connected = true;
            device_enumerated = false;
            device_address = (uint8_t)(*(uint32_t *)param);
            
            printf("[LPC55S69-Host] USB device attached. Address: %d\n", device_address);
            
            // Start the enumeration process
            memset(&device_info, 0, sizeof(device_info));
            enum_state = ENUM_STATE_GET_DEVICE_DESC;
            enum_retries = 0;
            
            // Trigger the first state of enumeration
            process_enumeration_state();
            break;
            
        case kUSB_HostEventDetach:
            device_connected = false;
            device_enumerated = false;
            device_address = 0;
            enum_state = ENUM_STATE_IDLE;
            
            printf("[LPC55S69-Host] USB device detached\n");
            break;
            
        case kUSB_HostEventEnumerationDone:
            // This event may be triggered by the NXP USB stack's internal enumeration
            // If we're using our own enumeration, we should check our state
            if (enum_state == ENUM_STATE_COMPLETE) {
                device_enumerated = true;
                printf("[LPC55S69-Host] USB enumeration completed successfully\n");
                printf("[LPC55S69-Host] Device VID:PID = %04x:%04x, Class: %02x\n",
                       device_info.vendor_id, device_info.product_id, device_info.device_class);
            } else {
                printf("[LPC55S69-Host] Received enumeration done event but our state is %d\n", enum_state);
                // Try to recover by moving to complete state
                enum_state = ENUM_STATE_COMPLETE;
                device_enumerated = true;
            }
            break;
            
        default:
            // Unhandled event
            break;
    }
}

/**
 * @brief Process the current enumeration state
 *
 * This function implements the state machine for USB device enumeration.
 */
static void process_enumeration_state(void)
{
    usb_status_t status = kStatus_USB_Success;
    
    if (!device_connected) {
        enum_state = ENUM_STATE_IDLE;
        return;
    }
    
    switch (enum_state) {
        case ENUM_STATE_IDLE:
            // Nothing to do
            break;
            
        case ENUM_STATE_GET_DEVICE_DESC:
            // Get the device descriptor (short version first to get max packet size)
            status = get_device_descriptor(false);
            if (status == kStatus_USB_Success) {
                parse_device_descriptor();
                enum_state = ENUM_STATE_SET_ADDRESS;
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_GET_FULL_DEVICE_DESC:
            // Get the full device descriptor now that we have the address set
            status = get_device_descriptor(true);
            if (status == kStatus_USB_Success) {
                parse_device_descriptor();
                enum_state = ENUM_STATE_GET_CONFIG_DESC;
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_SET_ADDRESS:
            // Set the device address (typically 1)
            status = set_device_address(1);
            if (status == kStatus_USB_Success) {
                enum_state = ENUM_STATE_GET_FULL_DEVICE_DESC;
                // Need a small delay to allow device to process SET_ADDRESS
                for (volatile int i = 0; i < 100000; i++) {
                    __asm("nop");
                }
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_GET_CONFIG_DESC:
            // Get the configuration descriptor header first
            status = get_config_descriptor(false);
            if (status == kStatus_USB_Success) {
                enum_state = ENUM_STATE_GET_FULL_CONFIG_DESC;
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_GET_FULL_CONFIG_DESC:
            // Get the full configuration descriptor including all interfaces
            status = get_config_descriptor(true);
            if (status == kStatus_USB_Success) {
                parse_config_descriptor();
                enum_state = ENUM_STATE_SET_CONFIGURATION;
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_SET_CONFIGURATION:
            // Set the configuration (typically 1)
            status = set_device_configuration(1);
            if (status == kStatus_USB_Success) {
                enum_state = ENUM_STATE_COMPLETE;
                device_enumerated = true;
                printf("[LPC55S69-Host] Enumeration completed successfully\n");
            } else {
                handle_enumeration_error(status);
            }
            break;
            
        case ENUM_STATE_COMPLETE:
            // Enumeration is complete, nothing more to do
            break;
    }
}

/**
 * @brief Get the device descriptor from the connected USB device
 *
 * @param full If true, get the full descriptor. If false, just get the first 8 bytes.
 * @return usb_status_t Status of the operation
 */
static usb_status_t get_device_descriptor(bool full)
{
    uint16_t length = full ? 18 : 8; // Full descriptor or just enough for max packet size
    
    // Setup packet for GET_DESCRIPTOR (Device)
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x80,  // Device-to-host, standard, device
        .bRequest = 0x06,       // GET_DESCRIPTOR
        .wValue = 0x0100,       // Device descriptor, index 0
        .wIndex = 0x0000,       // Not used for device descriptor
        .wLength = length
    };
    
    printf("[LPC55S69-Host] Getting %s device descriptor\n", full ? "full" : "short");
    
    // Send control transfer to get the descriptor
    int result = hurricane_hw_host_control_transfer(
        &setup,
        device_info.descriptor_buffer,
        length
    );
    
    if (result < 0 || result < length) {
        printf("[LPC55S69-Host] Failed to get device descriptor: %d\n", result);
        return kStatus_USB_Error;
    }
    
    return kStatus_USB_Success;
}

/**
 * @brief Parse the device descriptor to extract needed information
 */
static void parse_device_descriptor(void)
{
    // Check if we have at least the minimum device descriptor size
    if (device_info.descriptor_buffer[0] < 8) {
        printf("[LPC55S69-Host] Invalid device descriptor length: %d\n", device_info.descriptor_buffer[0]);
        return;
    }
    
    // Extract information from device descriptor
    uint8_t* desc = device_info.descriptor_buffer;
    
    // First 8 bytes are always available
    device_info.max_packet_size = desc[7];
    
    // Full descriptor fields
    if (desc[0] >= 18) {
        device_info.vendor_id = (desc[8] | (desc[9] << 8));
        device_info.product_id = (desc[10] | (desc[11] << 8));
        device_info.device_class = desc[4];
        device_info.device_subclass = desc[5];
        device_info.device_protocol = desc[6];
        device_info.num_configurations = desc[17];
    }
}

/**
 * @brief Get the configuration descriptor from the connected USB device
 *
 * @param full If true, get the full descriptor. If false, just get the header.
 * @return usb_status_t Status of the operation
 */
static usb_status_t get_config_descriptor(bool full)
{
    // For the header, we need at least 9 bytes (configuration descriptor header)
    uint16_t length = full ? enum_config_total_length : 9;
    
    // Setup packet for GET_DESCRIPTOR (Configuration)
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x80,  // Device-to-host, standard, device
        .bRequest = 0x06,       // GET_DESCRIPTOR
        .wValue = 0x0200,       // Configuration descriptor, index 0
        .wIndex = 0x0000,       // Not used for configuration descriptor
        .wLength = length
    };
    
    printf("[LPC55S69-Host] Getting %s configuration descriptor\n", full ? "full" : "header");
    
    // Send control transfer to get the descriptor
    int result = hurricane_hw_host_control_transfer(
        &setup,
        device_info.descriptor_buffer,
        length
    );
    
    if (result < 0 || result < 9) {
        printf("[LPC55S69-Host] Failed to get configuration descriptor: %d\n", result);
        return kStatus_USB_Error;
    }
    
    // If we're just getting the header, extract the total length for next request
    if (!full) {
        enum_config_total_length = (device_info.descriptor_buffer[2] | (device_info.descriptor_buffer[3] << 8));
        device_info.interface_count = device_info.descriptor_buffer[4];
        printf("[LPC55S69-Host] Config descriptor total length: %d, interfaces: %d\n",
               enum_config_total_length, device_info.interface_count);
    }
    
    return kStatus_USB_Success;
}

/**
 * @brief Parse the configuration descriptor to extract needed information
 */
static void parse_config_descriptor(void)
{
    // Basic validation
    if (device_info.descriptor_buffer[0] < 9) {
        printf("[LPC55S69-Host] Invalid config descriptor length: %d\n", device_info.descriptor_buffer[0]);
        return;
    }
    
    // Extract configuration value
    device_info.current_config = device_info.descriptor_buffer[5];
    
    // Full descriptor parsing could be implemented here if needed
    // For now, we just extract the basic configuration information
    printf("[LPC55S69-Host] Configuration value: %d\n", device_info.current_config);
}

/**
 * @brief Set the USB device address
 *
 * @param address The address to set
 * @return usb_status_t Status of the operation
 */
static usb_status_t set_device_address(uint8_t address)
{
    // Setup packet for SET_ADDRESS
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x00,  // Host-to-device, standard, device
        .bRequest = 0x05,       // SET_ADDRESS
        .wValue = address,      // New device address
        .wIndex = 0x0000,       // Not used for SET_ADDRESS
        .wLength = 0            // No data phase
    };
    
    printf("[LPC55S69-Host] Setting device address to %d\n", address);
    
    // Send control transfer to set the address
    int result = hurricane_hw_host_control_transfer(
        &setup,
        NULL,
        0
    );
    
    if (result < 0) {
        printf("[LPC55S69-Host] Failed to set device address: %d\n", result);
        return kStatus_USB_Error;
    }
    
    // Update our tracking of the device address
    device_address = address;
    
    return kStatus_USB_Success;
}

/**
 * @brief Set the USB device configuration
 *
 * @param config The configuration value to set
 * @return usb_status_t Status of the operation
 */
static usb_status_t set_device_configuration(uint8_t config)
{
    // Setup packet for SET_CONFIGURATION
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = 0x00,  // Host-to-device, standard, device
        .bRequest = 0x09,       // SET_CONFIGURATION
        .wValue = config,       // Configuration value
        .wIndex = 0x0000,       // Not used for SET_CONFIGURATION
        .wLength = 0            // No data phase
    };
    
    printf("[LPC55S69-Host] Setting device configuration to %d\n", config);
    
    // Send control transfer to set the configuration
    int result = hurricane_hw_host_control_transfer(
        &setup,
        NULL,
        0
    );
    
    if (result < 0) {
        printf("[LPC55S69-Host] Failed to set device configuration: %d\n", result);
        return kStatus_USB_Error;
    }
    
    return kStatus_USB_Success;
}

/**
 * @brief Handle errors during enumeration process
 *
 * @param status The error status code
 */
static void handle_enumeration_error(usb_status_t status)
{
    enum_retries++;
    
    if (enum_retries > 3) {
        printf("[LPC55S69-Host] Enumeration failed after multiple retries\n");
        enum_state = ENUM_STATE_IDLE;
        return;
    }
    
    printf("[LPC55S69-Host] Retrying enumeration state %d (retry #%d)\n", enum_state, enum_retries);
    // Stay in the same state to retry
}

static void USB_HostHidCallback(void* param, 
                              uint8_t* buffer, 
                              uint32_t length)
{
    // Process HID data if needed
    
    // For now, just print transfer completion
    printf("[LPC55S69-Host] HID transfer complete. Length: %d\n", length);
}
