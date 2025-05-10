/**
 * @file usb_hw_hal_device_lpc55s69.c
 * @brief LPC55S69-specific USB device HAL implementation
 * 
 * Implements device-mode USB HAL for LPC55S69 (Full Speed USB0)
 */
#include "hurricane_hw_hal.h"
// Forward declare types needed by USB headers to avoid dependency issues
#ifndef USB_DEVICE_CONFIG_H
typedef void* usb_device_controller_handle;
typedef uint32_t usb_device_control_type_t;
#endif

// Define missing clock and reset identifiers if not provided by SDK
#ifndef kCLOCK_Usb0Device
#define kCLOCK_Usb0Device kCLOCK_Usbd0
#endif

#ifndef kUSB0_RST_SHIFT_RSTn
#define kUSB0_RST_SHIFT_RSTn kUSB0D_RST_SHIFT_RSTn
#endif

#include "hurricane_hw_hal.h"
#include "core/usb_interface_manager.h" // MAX_ENDPOINTS_PER_INTERFACE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// NXP SDK includes
// 1. Device configuration (macros like USB_DEVICE_CONFIG_INTERFACES)
#include "usb_device_config.h" 

// 2. Core CMSIS and device registers
#include "fsl_device_registers.h" // Includes LPC55S69_cm33_core0.h, which includes core_cm33.h

// 3. Basic types and common utilities
#include "fsl_common.h"

// 4. Peripheral drivers (clocks, power, reset)
#include "fsl_power.h"
#include "fsl_clock.h" // For kCLOCK_Usb0Device, kCLOCK_Usb0PhyClk etc.
#include "fsl_reset.h" // For kUSB0_RST_SHIFT_RSTn

// 5. USB Core & Device Stack Headers
#include "usb.h"                  // Basic USB types and definitions
#include "usb_device.h"         // Core device stack, defines usb_device_controller_handle, usb_device_control_type_t
#include "usb_device_ch9.h"       // Chapter 9 (standard requests)
#include "usb_device_class.h"     // Device class handling

// 6. USB Controller-Specific Headers (after usb_device.h)
#include "usb_device_lpcip3511.h" // For lpcip3511 controller specific functions

// If BOARD_USB_PHY_D_CAL is not in fsl_usb_phy.h, it might be in a board.h or usb_phy_config.h
// For example, you might need:
// #include "board.h" 
// or ensure usb_phy_config.h (if used) has the definition.
// For now, we assume fsl_usb_phy.h or an implicitly included header provides it.

//==============================================================================
// Private definitions and variables
//==============================================================================

static usb_device_handle device_handle;

static bool device_initialized = false;
static bool device_attached    = false;

// Removed unused variables: current_configuration, current_interface, current_alternate_setting

// Control request state tracking
typedef struct {
    bool response_pending;
    bool response_ready;
    bool handled;
    uint8_t buffer[512];
    uint16_t length;
    uint32_t timeout_ms;
    uint32_t request_time;

    struct {
        uint8_t interface_num;
        hurricane_usb_setup_packet_t setup;
    } request_info;
} control_request_state_t;

// Removed unused variable: control_state

// Removed unused forward declaration for control response

// External event notification
extern bool hurricane_interface_notify_event_with_response(
    hurricane_usb_event_t event,
    uint8_t interface_num,
    void *event_data,
    void (*response_cb)(uint8_t interface_num, bool handled, void *buffer, uint16_t length)
);

// Dynamic interface tracking
typedef struct {
    bool configured;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t num_endpoints;

    struct {
        uint8_t ep_address;
        uint8_t ep_attributes;
        uint16_t ep_max_packet_size;
        uint8_t ep_interval;
    } endpoints[MAX_ENDPOINTS_PER_INTERFACE];
} lpc55s69_interface_config_t;

static lpc55s69_interface_config_t configured_interfaces[USB_DEVICE_CONFIG_INTERFACES];

// Descriptors
static uint8_t *g_device_descriptor         = NULL;
static uint16_t g_device_descriptor_length  = 0;
static uint8_t *g_config_descriptor         = NULL;
static uint16_t g_config_descriptor_length  = 0;
static uint8_t *g_hid_report_descriptor     = NULL;
static uint16_t g_hid_report_descriptor_length = 0;

int hurricane_hw_device_set_descriptors(const uint8_t *device_desc, uint16_t device_len,
                                       const uint8_t *config_desc, uint16_t config_len) {
    g_device_descriptor = (uint8_t*)device_desc;
    g_device_descriptor_length = device_len;
    g_config_descriptor = (uint8_t*)config_desc;
    g_config_descriptor_length = config_len;
    return 0; // Added return
}

int hurricane_hw_device_set_hid_report_descriptor(const uint8_t *report_desc, uint16_t report_len) {
    g_hid_report_descriptor = (uint8_t*)report_desc;
    g_hid_report_descriptor_length = report_len;
    return 0; // Added return
}

// String Descriptors
// Removed unused string descriptor arrays

// User callbacks
static void (*g_set_configuration_callback)(uint8_t configuration) = NULL;
static void (*g_set_interface_callback)(uint8_t interface, uint8_t alt_setting) = NULL;

// External hardware setup
// extern void usb_device_hw_init(void); // Already part of NXP SDK, typically called by USB_DeviceInit
// extern void USB_DeviceIsrEnable(void); // This is controller specific, e.g. USB_DeviceLpcIp3511EnableInterrupts

//==============================================================================
// Forward declarations
//==============================================================================

static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
// Removed unused static function declarations

//==============================================================================
// USB Device Callback Implementation
//==============================================================================

/**
 * @brief USB Device callback function implementation
 *
 * This function is referenced in USB_DeviceInit() call at line 201
 * and handles device events from the USB controller.
 */
static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    switch (event) {
        case kUSB_DeviceEventBusReset:
            /* Device reset detected */
            device_attached = true;
            break;
            
        case kUSB_DeviceEventAttach:
            /* USB device attached */
            device_attached = true;
            break;
            
        case kUSB_DeviceEventDetach:
            /* USB device detached */
            device_attached = false;
            break;
            
        default:
            /* Handle other events as needed */
            break;
    }
    
    return kStatus_USB_Success;
}

//==============================================================================
// Public HAL functions
//==============================================================================

static void init_interface_config(void) {
    memset(configured_interfaces, 0, sizeof(configured_interfaces));
    for (unsigned int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
        configured_interfaces[i].configured = false;
        configured_interfaces[i].num_endpoints = 0;
    }
}

void hurricane_hw_device_init(void) {
    if (device_initialized) {
        printf("[LPC55S69-Device] Device already initialized\n");
        return;
    }

    usb_status_t status = kStatus_USB_Success;

    // Enable USB0 device clock
    CLOCK_EnableClock(kCLOCK_Usb0Device);
    // Set USB0 clock divider
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1, true);
    // Attach FRO to USB0
    CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
    // Reset USB0 controller
    RESET_PeripheralReset(kUSB0_RST_SHIFT_RSTn);
    // Power up USB0 PHY
    POWER_DisablePD(kPDRUNCFG_PD_USB0_PHY);

    // Set interrupt priority and enable IRQ
    NVIC_SetPriority(USB0_IRQn, 3);
    NVIC_EnableIRQ(USB0_IRQn);

    // Initialize USB device stack
    status = USB_DeviceInit(USB_DEVICE_CONFIG_CONTROLLER_ID, USB_DeviceCallback, &device_handle);
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] USB device controller init failed\n");
        return;
    }

    device_initialized = true;
    device_attached = false;
    init_interface_config();

    printf("[LPC55S69-Device] USB device initialized (Full-Speed)\n");
}

// Add MCUXpresso-compatible interrupt handler with flag cleanup
void USB0_IRQHandler(void) { // Renamed to match SDK's expected ISR name for USB0
    // hurricane_usb_isr_handler(); // This seems like a generic handler, ensure it's needed or remove
    USB_DeviceLpcIp3511IsrFunction(device_handle); // Use controller-specific ISR function
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping 
      exception return operation might vector to incorrect interrupt. */
    __DSB();
}

void hurricane_hw_device_poll(void) {
    // Interrupt-driven by default; poll if needed.
}

int hurricane_hw_device_host_connected(void) {
    return device_attached ? 1 : 0;
}

void hurricane_hw_device_reset(void) {
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return;
    }

    USB_DeviceStop(device_handle);
    for (volatile int i = 0; i < 10000000; i++) __asm("nop");
    USB_DeviceRun(device_handle);

    printf("[LPC55S69-Device] Device reset completed\n");
}

// Add status tracking for control transfers
static usb_status_t last_control_status = kStatus_USB_Success;

int hurricane_hw_device_control_response(const hurricane_usb_setup_packet_t *setup, void *buffer, uint16_t length) {
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return -1;
    }

    usb_status_t status;
    if (setup->bmRequestType & 0x80) {
        status = USB_DeviceSendRequest(device_handle, 0, buffer, length);
    } else {
        status = USB_DeviceRecvRequest(device_handle, 0, buffer, length);
    }

    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Control transfer failed (status=%d)\n", status);
        return -1;
    }

    last_control_status = status;
    return length;
}

int hurricane_hw_device_interrupt_in_transfer(uint8_t endpoint, void *buffer, uint16_t length) {
    if (!device_initialized || !device_attached) {
        printf("[LPC55S69-Device] Not ready for IN transfer\n");
        return -1;
    }

    if (!(endpoint & 0x80)) {
        printf("[LPC55S69-Device] Invalid IN endpoint 0x%02x\n", endpoint);
        return -1;
    }

    uint8_t ep_num = endpoint & 0x0F;
    usb_status_t status = USB_DeviceSendRequest(device_handle, ep_num, buffer, length);

    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] IN transfer failed (status=%d)\n", status);
        return -1;
    }

    return length;
}

int hurricane_hw_device_interrupt_out_transfer(uint8_t endpoint, void *buffer, uint16_t length) {
    if (!device_initialized || !device_attached) {
        printf("[LPC55S69-Device] Not ready for OUT transfer\n");
        return -1;
    }

    if (endpoint & 0x80) {
        printf("[LPC55S69-Device] Invalid OUT endpoint 0x%02x\n", endpoint);
        return -1;
    }

    uint8_t ep_num = endpoint & 0x0F;
    usb_status_t status = USB_DeviceRecvRequest(device_handle, ep_num, buffer, length);

    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] OUT transfer failed (status=%d)\n", status);
        return -1;
    }

    return length;
}

void hurricane_hw_device_set_configuration_callback(void (*callback)(uint8_t configuration)) {
    g_set_configuration_callback = callback;
}

void hurricane_hw_device_set_interface_callback(void (*callback)(uint8_t interface, uint8_t alt_setting)) {
    g_set_interface_callback = callback;
}

// Aligned with hurricane_hw_hal.h
int hurricane_hw_device_configure_interface(uint8_t interface_num, 
                                            uint8_t interface_class, 
                                            uint8_t interface_subclass, 
                                            uint8_t interface_protocol) {
    if (!device_handle || interface_num >= USB_DEVICE_CONFIG_INTERFACES) {
        return -1; // Or some error code
    }

    // Store configuration - this part might need more logic if you dynamically build descriptors
    configured_interfaces[interface_num].configured = true;
    configured_interfaces[interface_num].interface_class = interface_class;
    configured_interfaces[interface_num].interface_subclass = interface_subclass;
    configured_interfaces[interface_num].interface_protocol = interface_protocol;
    
    // The actual SET_INTERFACE is usually handled by the host. 
    // This function in device mode typically prepares the interface for use.
    // If you need to actively tell the NXP stack about this interface configuration
    // beyond just storing it for descriptor generation, that logic would go here.
    // For now, we assume this function's role is to ready the HAL's internal state.

    // Rebuild configuration descriptor if it's dynamic based on these calls
    // rebuild_configuration_descriptor(); // If you have such a function

    return 0; // Success
}

// Aligned with hurricane_hw_hal.h
int hurricane_hw_device_configure_endpoint(uint8_t interface_num,
                                           uint8_t ep_address,
                                           uint8_t ep_attributes,
                                           uint16_t ep_max_packet_size,
                                           uint8_t ep_interval) {
    if (!device_handle || interface_num >= USB_DEVICE_CONFIG_INTERFACES) {
        return -1;
    }

    usb_device_endpoint_init_struct_t ep_init = {0};
    
    // Deinit endpoint first if it was previously configured
    // The NXP stack might handle re-configuration gracefully, but explicit deinit is safer.
    USB_DeviceDeinitEndpoint(device_handle, ep_address);
    
    // Configure new endpoint settings
    ep_init.endpointAddress = ep_address;
    ep_init.maxPacketSize = ep_max_packet_size;
    ep_init.transferType = ep_attributes & USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TYPE_MASK; // Corrected Macro
    ep_init.interval = ep_interval; // Interval is relevant for Interrupt and Isochronous
    ep_init.zlt = 0; // Zero Length Termination, typically 0 for bulk/interrupt unless specifically needed

    usb_device_endpoint_callback_struct_t ep_callback = {0};
    ep_callback.callbackFn = NULL;
    ep_callback.callbackParam = NULL;

    usb_status_t status = USB_DeviceInitEndpoint(device_handle, &ep_init, &ep_callback);

    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Endpoint 0x%02X init failed, status=%d\n", ep_address, status);
        return -1; // Or map usb_status_t to an error code
    }
    
    // Store endpoint config if your HAL needs it
    if (configured_interfaces[interface_num].num_endpoints < MAX_ENDPOINTS_PER_INTERFACE) {
        lpc55s69_interface_config_t* if_cfg = &configured_interfaces[interface_num];
        if_cfg->endpoints[if_cfg->num_endpoints].ep_address = ep_address;
        if_cfg->endpoints[if_cfg->num_endpoints].ep_attributes = ep_attributes;
        if_cfg->endpoints[if_cfg->num_endpoints].ep_max_packet_size = ep_max_packet_size;
        if_cfg->endpoints[if_cfg->num_endpoints].ep_interval = ep_interval;
        if_cfg->num_endpoints++;
    }

    // Prime endpoint for reception if OUT and not control
    if ((ep_address & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) == USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT &&
        (ep_address & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK) != 0) {
        USB_DeviceRecvRequest(device_handle,
            (ep_address & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK),
            NULL, 0); // Prime with a zero-byte receive, or a buffer if you have one ready
    }
    return 0; // Success
}

// The following are the original functions that had conflicting signatures.
// They are now commented out or removed as the HAL-compliant versions are above.
/*
void hurricane_hw_device_configure_interface(uint8_t interface_num, const usb_interface_descriptor_t* interface_desc) {
    // Configure interface settings
    // USB_DeviceSetInterface(interface_num, interface_desc->bAlternateSetting); // USB_DeviceSetInterface is not a standard NXP SDK API for this purpose
    
    // Enable endpoint configurations from descriptor
    const usb_endpoint_descriptor_t* ep_desc = (usb_endpoint_descriptor_t*)(interface_desc + 1);
    for (uint8_t i = 0; i < interface_desc->bNumEndpoints; i++) {
        // This old call is incompatible with the HAL
        // hurricane_hw_device_configure_endpoint(&ep_desc[i]); 
        
        // New call would be:
        hurricane_hw_device_configure_endpoint(interface_num, 
                                               ep_desc->bEndpointAddress, 
                                               ep_desc->bmAttributes, 
                                               ep_desc->wMaxPacketSize, 
                                               ep_desc->bInterval);
        ep_desc = (usb_endpoint_descriptor_t*)((uint8_t*)ep_desc + ep_desc->bLength);
    }
}

void hurricane_hw_device_configure_endpoint(const usb_endpoint_descriptor_t* ep_desc) {
    usb_device_endpoint_init_struct_t ep_init = {0};
    // usb_device_endpoint_uninit_struct_t ep_uninit; // Incorrect type
    
    // First uninitialize endpoint if already configured
    // ep_uninit.epAddress = ep_desc->bEndpointAddress; // Member does not exist on incorrect type
    USB_DeviceDeinitEndpoint(device_handle, ep_desc->bEndpointAddress); // Pass address directly
    
    // Configure new endpoint settings
    ep_init.endpointAddress = ep_desc->bEndpointAddress; // Corrected member name
    ep_init.maxPacketSize = ep_desc->wMaxPacketSize;
    ep_init.transferType = ep_desc->bmAttributes & USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TYPE_MASK; // Corrected Macro
    
    if (ep_init.transferType == USB_ENDPOINT_INTERRUPT) { // USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TYPE_INTERRUPT
        ep_init.interval = ep_desc->bInterval;
    }
    ep_init.zlt = 0; // Default ZLT
    
    // Align with MCUXpresso endpoint state management
    // USB_DeviceCancel(device_handle, ep_init.endpointAddress); // epAddress was wrong here
    // USB_DeviceFlush(device_handle, ep_init.endpointAddress); // USB_DeviceFlush not found, and epAddress was wrong
    
    // The NXP SDK might not have a direct USB_DeviceFlush. Clearing stall and ensuring endpoint is ready is typical.
    // USB_DeviceUnstallEndpoint(device_handle, ep_init.endpointAddress); // Example if needed

    usb_status_t status = USB_DeviceInitEndpoint(device_handle, &ep_init, 1); // Added flags argument
    if (status != kStatus_USB_Success) {
         printf(\"[LPC55S69-Device] Endpoint 0x%02X init failed, status=%d\\n\", ep_init.endpointAddress, status);
    }
    
    // Prime endpoint for reception if OUT
    if ((ep_init.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) == USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT) {
        USB_DeviceRecvRequest(device_handle,
            (ep_init.endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK),
            NULL, 0); // Prime with a zero-byte receive
    }
}
*/

//==============================================================================
// End of file
//==============================================================================
