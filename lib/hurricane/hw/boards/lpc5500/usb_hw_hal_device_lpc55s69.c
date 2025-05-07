/**
 * @file usb_hw_hal_device_lpc55s69.c
 * @brief LPC55S69-specific USB device HAL implementation
 * 
 * Implements device-mode USB HAL for LPC55S69 (Full Speed USB0)
 */

#include "hurricane_hw_hal.h"
#include "core/usb_interface_manager.h" // MAX_ENDPOINTS_PER_INTERFACE

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
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_ch9.h"
#include "usb_device_class.h"

//==============================================================================
// Private definitions and variables
//==============================================================================

static usb_device_handle device_handle;

static bool device_initialized = false;
static bool device_attached    = false;

static uint8_t current_configuration      = 0;
static uint8_t current_interface          = 0;
static uint8_t current_alternate_setting  = 0;

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

static control_request_state_t control_state;

// Forward declaration for control response
static void control_request_response_callback(uint8_t interface_num, bool handled, void *buffer, uint16_t length);

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

void hurricane_hw_device_set_descriptors(const uint8_t *device_desc, uint16_t device_len,
                                       const uint8_t *config_desc, uint16_t config_len) {
    g_device_descriptor = (uint8_t*)device_desc;
    g_device_descriptor_length = device_len;
    g_config_descriptor = (uint8_t*)config_desc;
    g_config_descriptor_length = config_len;
}

void hurricane_hw_device_set_hid_report_descriptor(const uint8_t *report_desc, uint16_t report_len) {
    g_hid_report_descriptor = (uint8_t*)report_desc;
    g_hid_report_descriptor_length = report_len;
}

// String Descriptors
static uint8_t *g_string_descriptors[USB_DEVICE_CONFIG_STRING_COUNT] = { NULL };
static uint16_t g_string_descriptor_lengths[USB_DEVICE_CONFIG_STRING_COUNT] = { 0 };

// User callbacks
static void (*g_set_configuration_callback)(uint8_t configuration) = NULL;
static void (*g_set_interface_callback)(uint8_t interface, uint8_t alt_setting) = NULL;

// External hardware setup
extern void usb_device_hw_init(void);
extern void USB_DeviceIsrEnable(void);

//==============================================================================
// Forward declarations
//==============================================================================

static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
static usb_status_t USB_DeviceSetupPacketHandler(usb_device_handle handle, usb_setup_struct_t *setup, uint32_t *length, uint8_t **buffer);
static int rebuild_configuration_descriptor(void);
static bool is_control_request_timeout(void);
static usb_status_t complete_control_transfer(usb_setup_struct_t *setup, uint8_t *buffer, uint32_t length);

//==============================================================================
// Public HAL functions
//==============================================================================

static void init_interface_config(void) {
    memset(configured_interfaces, 0, sizeof(configured_interfaces));
    for (int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
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
    
    // LPC55S69-specific clock initialization
    CLOCK_EnableClock(kCLOCK_Usb0Device);
    CLOCK_SetClkDiv(kCLOCK_DivUsb0Clk, 1, true);
    CLOCK_AttachClk(kFRO_HF_to_USB0_CLK);
    RESET_PeripheralReset(kUSB0_RST_SHIFT_RSTn);
    
    // MCUXpresso-compatible interrupt configuration
    NVIC_SetPriority(USB0_IRQn, 3);
    NVIC_EnableIRQ(USB0_IRQn);
    
    // PHY initialization
    USB_DeviceEhciPhyInit(USB0, BOARD_USB_PHY_D_CAL, 0);
    
    usb_device_hw_init();

    status = USB_DeviceInit(USB0, USB_DeviceCallback, &device_handle);
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] USB device controller init failed\n");
        return;
    }

    USB_DeviceIsrEnable();
    USB_DeviceRun(device_handle);

    device_initialized = true;
    device_attached = false;
    init_interface_config();

    printf("[LPC55S69-Device] USB device initialized (Full-Speed)\n");
}

// Add MCUXpresso-compatible interrupt handler with flag cleanup
void USB_DeviceIrqHandler(void) {
    hurricane_usb_isr_handler();
    USB_DeviceEhciIsrFunction(device_handle, NULL);
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

void hurricane_hw_device_configure_interface(uint8_t interface_num, const usb_interface_descriptor_t* interface_desc) {
    // Configure interface settings
    USB_DeviceSetInterface(interface_num, interface_desc->bAlternateSetting);
    
    // Enable endpoint configurations from descriptor
    const usb_endpoint_descriptor_t* ep_desc = (usb_endpoint_descriptor_t*)(interface_desc + 1);
    for (uint8_t i = 0; i < interface_desc->bNumEndpoints; i++) {
        hurricane_hw_device_configure_endpoint(&ep_desc[i]);
        ep_desc = (usb_endpoint_descriptor_t*)((uint8_t*)ep_desc + ep_desc->bLength);
    }
}

void hurricane_hw_device_configure_endpoint(const usb_endpoint_descriptor_t* ep_desc) {
    usb_device_endpoint_init_struct_t ep_init;
    usb_device_endpoint_uninit_struct_t ep_uninit;
    
    // First uninitialize endpoint if already configured
    ep_uninit.epAddress = ep_desc->bEndpointAddress;
    USB_DeviceDeinitEndpoint(device_handle, &ep_uninit);
    
    // Configure new endpoint settings
    ep_init.epAddress = ep_desc->bEndpointAddress;
    ep_init.maxPacketSize = ep_desc->wMaxPacketSize;
    ep_init.transferType = ep_desc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
    
    if (ep_init.transferType == USB_ENDPOINT_INTERRUPT) {
        ep_init.interval = ep_desc->bInterval;
    }
    
    // Align with MCUXpresso endpoint state management
    USB_DeviceCancel(device_handle, ep_init.epAddress);
    USB_DeviceFlush(device_handle, ep_init.epAddress);
    USB_DeviceInitEndpoint(device_handle, &ep_init);
    
    // Prime endpoint for reception if OUT
    if ((ep_init.epAddress & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_OUT) {
        USB_DeviceRecvRequest(device_handle,
            (ep_init.epAddress & USB_ENDPOINT_NUMBER_MASK),
            NULL, 0);
    }
}
