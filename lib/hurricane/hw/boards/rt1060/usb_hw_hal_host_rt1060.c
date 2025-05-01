/**
 * @file usb_hw_hal_host_rt1060.c
 * @brief RT1060-specific USB host HAL implementation
 * 
 * This file implements the host-mode USB HAL functions for the MIMXRT1060 platform
 * using the NXP SDK. It uses the first USB controller (USB1) in host mode.
 */

#include "hw/hurricane_hw_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// NXP SDK includes
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
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

//==============================================================================
// Public HAL functions
//==============================================================================

void hurricane_hw_host_init(void)
{
    if (host_initialized) {
        printf("[RT1060-Host] Host already initialized\n");
        return;
    }

    uint8_t irqNumber;
    uint8_t usbHostKhciIrq;
    usb_status_t status = kStatus_USB_Success;

    // USB1 PHY initialization
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
    };

    CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
    CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);

    // Initialize USB PHY
    USB_EhciPhyInit(kUSB_ControllerEhci0, 0, &phyConfig);

    // Get USB EHCI IRQ
    irqNumber = USB_HOST_INTERRUPT_PRIORITY;
    usbHostKhciIrq = USBHS0_IRQn;

    // Install IRQ handler
    NVIC_SetPriority((IRQn_Type)usbHostKhciIrq, irqNumber);

    // Initialize host controller
    status = USB_HostInit(kUSB_ControllerEhci0, &host_handle, USB_HostCallback);
    if (kStatus_USB_Success != status) {
        printf("[RT1060-Host] USB host controller initialization failed\n");
        return;
    }

    // Enable IRQ
    NVIC_EnableIRQ((IRQn_Type)usbHostKhciIrq);

    host_initialized = true;
    device_connected = false;
    device_enumerated = false;
    
    printf("[RT1060-Host] USB host initialized\n");
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
        printf("[RT1060-Host] Host not initialized\n");
        return;
    }
    
    usb_status_t status = USB_HostResetDevice(host_handle, device_address);
    
    if (status != kStatus_USB_Success) {
        printf("[RT1060-Host] Failed to reset USB bus: %d\n", status);
    } else {
        printf("[RT1060-Host] USB bus reset initiated\n");
    }
}

int hurricane_hw_host_control_transfer(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length)
{
    if (!host_initialized || !device_connected || !device_enumerated) {
        printf("[RT1060-Host] Host not initialized or device not connected\n");
        return -1;
    }
    
    if (!setup) {
        printf("[RT1060-Host] Invalid setup packet\n");
        return -1;
    }
    
    usb_host_transfer_t* transfer;
    
    // Allocate transfer
    if (USB_HostMallocTransfer(host_handle, &transfer) != kStatus_USB_Success) {
        printf("[RT1060-Host] Failed to allocate transfer\n");
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
        printf("[RT1060-Host] Failed to send control request: %d\n", status);
        return -1;
    }
    
    // Wait for transfer completion (in a real implementation, this would be asynchronous)
    // For simplicity, we're using a blocking approach here
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
        printf("[RT1060-Host] Control transfer timed out\n");
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
        printf("[RT1060-Host] Host not initialized or device not connected\n");
        return -1;
    }
    
    // Make sure this is an IN endpoint
    if (!(endpoint & 0x80)) {
        printf("[RT1060-Host] Invalid IN endpoint 0x%02x\n", endpoint);
        return -1;
    }
    
    usb_host_transfer_t* transfer;
    
    // Allocate transfer
    if (USB_HostMallocTransfer(host_handle, &transfer) != kStatus_USB_Success) {
        printf("[RT1060-Host] Failed to allocate transfer\n");
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
        printf("[RT1060-Host] Failed to send interrupt request: %d\n", status);
        return -1;
    }
    
    // Wait for transfer completion (in a real implementation, this would be asynchronous)
    // For simplicity, we're using a blocking approach here
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
        printf("[RT1060-Host] Interrupt transfer timed out\n");
        USB_HostCancelTransfer(host_handle, device_address, transfer);
        return -1;
    }
    
    // Return the number of bytes transferred
    return transfer->transferSofar;
}

//==============================================================================
// Backward compatibility functions
//==============================================================================

void hurricane_hw_init(void)
{
    printf("[RT1060] Initializing dual USB stack\n");
    
    // Initialize both host and device stacks
    hurricane_hw_host_init();
    hurricane_hw_device_init();
    
    printf("[RT1060] Dual USB stack initialized\n");
    printf("[RT1060]   - USB1: Host Mode (Type A connector)\n");
    printf("[RT1060]   - USB2: Device Mode (Micro B connector)\n");
}

void hurricane_hw_poll(void)
{
    // Poll both host and device stacks
    hurricane_hw_host_poll();
    hurricane_hw_device_poll();
}

/**
 * @brief Synchronize with companion controller
 *
 * This function ensures proper synchronization between host and device
 * controllers when they need to coordinate operations.
 */
void hurricane_hw_sync_controllers(void)
{
    // In this implementation, no specific sync is needed between controllers
    // because USB1 and USB2 operate independently on the RT1060
    
    // In case specific synchronization is needed in the future,
    // it can be implemented here
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
            
            printf("[RT1060-Host] USB device attached. Address: %d\n", device_address);
            
            // In a real implementation, this would trigger the enumeration process
            // For simplicity, we're just marking the device as enumerated here
            device_enumerated = true;
            break;
            
        case kUSB_HostEventDetach:
            device_connected = false;
            device_enumerated = false;
            device_address = 0;
            
            printf("[RT1060-Host] USB device detached\n");
            break;
            
        case kUSB_HostEventEnumerationDone:
            device_enumerated = true;
            printf("[RT1060-Host] USB enumeration completed\n");
            
            // At this point in a real implementation, we would:
            // 1. Get device descriptors
            // 2. Configure the device
            // 3. Identify device classes and interfaces
            // 4. Connect appropriate class drivers
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
    printf("[RT1060-Host] HID transfer complete. Length: %d\n", length);
}