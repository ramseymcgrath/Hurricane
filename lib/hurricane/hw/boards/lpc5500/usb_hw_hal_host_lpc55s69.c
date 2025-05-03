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
    
    printf("[LPC55S69-Host] USB host initialized (High-Speed)\n");
}

void hurricane_hw_host_poll(void)
{
    if (!host_initialized) {
        return;
    }
    
    // Call USB host tasks
    USB_HostTaskFn(host_handle);
    
    // Additional polling logic as needed
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
            
            // TODO this should actually trigger the enumeration process
            // this is just making the preprogrammed enumarated
            device_enumerated = true;
            break;
            
        case kUSB_HostEventDetach:
            device_connected = false;
            device_enumerated = false;
            device_address = 0;
            
            printf("[LPC55S69-Host] USB device detached\n");
            break;
            
        case kUSB_HostEventEnumerationDone:
            device_enumerated = true;
            printf("[LPC55S69-Host] USB enumeration completed\n");
            
            // full flow:
            // Get device descriptors
            // Configure the device
            // Identify device classes and interfaces
            // Connect appropriate class drivers
            break;
            
        default:
            // Unhandled event
            break;
    }
}

static void USB_HostHidCallback(void* param, 
                              uint8_t* buffer, 
                              uint32_t length)
{
    // Process HID data if needed
    
    // For now, just print transfer completion
    printf("[LPC55S69-Host] HID transfer complete. Length: %d\n", length);
}
