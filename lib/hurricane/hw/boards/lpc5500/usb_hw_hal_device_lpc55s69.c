/**
 * @file usb_hw_hal_device_lpc55s69.c
 * @brief LPC55S69-specific USB device HAL implementation
 * 
 * This file implements the device-mode USB HAL functions for the LPC55S69 platform
 * using the NXP SDK. It uses the USB0 controller in device mode (Full Speed).
 */

#include "hw/hurricane_hw_hal.h"
#include "core/usb_interface_manager.h" // For MAX_ENDPOINTS_PER_INTERFACE
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
#include "usb_phy.h"

// Include our dynamic interface manager
#include "core/usb_interface_manager.h"

//==============================================================================
// Private definitions and variables
//==============================================================================

// USB device controller handle
static usb_device_handle device_handle;

// Device stack status
static bool device_initialized = false;
static bool device_attached = false;
static uint8_t current_configuration = 0;
static uint8_t current_interface = 0;
static uint8_t current_alternate_setting = 0;

// Control request response handling
typedef struct {
    bool response_pending;                 // Indicates a response is pending
    bool response_ready;                   // Indicates a response is ready
    uint8_t buffer[512];                   // Buffer for response data
    uint16_t length;                       // Length of response data
    bool handled;                          // Whether the request was handled
    struct {
        uint8_t interface_num;             // Interface that handled the request
        hurricane_usb_setup_packet_t setup;  // Setup packet for the request
    } request_info;
    uint32_t timeout_ms;                   // Timeout in milliseconds
    uint32_t request_time;                 // Time when request was made
} control_request_state_t;

static control_request_state_t control_state;

// Forward declarations
static void control_request_response_callback(uint8_t interface_num, bool handled, void* buffer, uint16_t length);
extern bool hurricane_interface_notify_event_with_response(
    hurricane_usb_event_t event,
    uint8_t interface_num,
    void* event_data,
    void (*response_cb)(uint8_t interface_num, bool handled, void* buffer, uint16_t length));

// Dynamic interface configuration tracking
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

// Device descriptors
static uint8_t* g_device_descriptor = NULL;
static uint16_t g_device_descriptor_length = 0;
static uint8_t* g_config_descriptor = NULL;
static uint16_t g_config_descriptor_length = 0;
static uint8_t* g_hid_report_descriptor = NULL;
static uint16_t g_hid_report_descriptor_length = 0;

// String descriptors
static uint8_t* g_string_descriptors[USB_DEVICE_CONFIG_STRING_COUNT] = {NULL};
static uint16_t g_string_descriptor_lengths[USB_DEVICE_CONFIG_STRING_COUNT] = {0};

// Callbacks
static void (*g_set_configuration_callback)(uint8_t configuration) = NULL;
static void (*g_set_interface_callback)(uint8_t interface, uint8_t alt_setting) = NULL;

// Forward declarations
static usb_status_t USB_DeviceCallback(usb_device_handle handle, 
                                      uint32_t event, 
                                      void *param);
static usb_status_t USB_DeviceSetupPacketHandler(usb_device_handle handle, 
                                                usb_setup_struct_t *setup, 
                                                uint32_t *length, 
                                                uint8_t **buffer);

// External declarations from initialization file
extern void usb_device_hw_init(void);
extern void USB_DeviceIsrEnable(void);

//==============================================================================
// Public HAL functions
//==============================================================================

/**
 * @brief Initialize dynamic interface tracking
 */
static void init_interface_config(void)
{
    memset(configured_interfaces, 0, sizeof(configured_interfaces));
    for (int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
        configured_interfaces[i].configured = false;
        configured_interfaces[i].num_endpoints = 0;
        // No need to explicitly initialize the endpoints array
        // as memset has already zeroed it out
    }
}

static void init_interface_config(void);  // Forward declaration

void hurricane_hw_device_init(void)
{
    if (device_initialized) {
        printf("[LPC55S69-Device] Device already initialized\n");
        return;
    }

    usb_status_t status = kStatus_USB_Success;

    // Initialize USB0 hardware (Full-Speed)
    usb_device_hw_init();

    // Initialize USB device controller
    status = USB_DeviceInit(kUSB_ControllerLpcIp3511Fs0, USB_DeviceCallback, &device_handle);
    if (kStatus_USB_Success != status) {
        printf("[LPC55S69-Device] USB device controller initialization failed\n");
        return;
    }

    // Enable IRQ
    USB_DeviceIsrEnable();

    // Start the USB device
    USB_DeviceRun(device_handle);

    device_initialized = true;
    device_attached = false;
    
    // Initialize interface configuration tracking
    init_interface_config();
    
    printf("[LPC55S69-Device] USB device initialized (Full-Speed)\n");
}

void hurricane_hw_device_poll(void)
{
    // Most of the USB device handling is done in interrupts
    // This function can be used for any periodic tasks related to device mode
}

int hurricane_hw_device_host_connected(void)
{
    return device_attached ? 1 : 0;
}

void hurricane_hw_device_reset(void)
{
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return;
    }

    // This simulates a disconnect/connect cycle
    USB_DeviceStop(device_handle);
    
    // Brief delay
    for (volatile int i = 0; i < 10000000; i++) {
        __asm("nop");
    }
    
    USB_DeviceRun(device_handle);
    
    printf("[LPC55S69-Device] Device reset completed\n");
}

int hurricane_hw_device_control_response(
    const hurricane_usb_setup_packet_t* setup,
    void* buffer,
    uint16_t length)
{
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return -1;
    }

    usb_status_t status;
    
    if (setup->bmRequestType & 0x80) {
        // Device-to-host (IN) transfer
        status = USB_DeviceSendRequest(device_handle, 0, buffer, length);
    } else {
        // Host-to-device (OUT) transfer
        status = USB_DeviceRecvRequest(device_handle, 0, buffer, length);
    }
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Control transfer failed with status %d\n", status);
        return -1;
    }
    
    return length;
}

int hurricane_hw_device_interrupt_in_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length)
{
    if (!device_initialized || !device_attached) {
        printf("[LPC55S69-Device] Device not initialized or not attached\n");
        return -1;
    }
    
    // Make sure this is an IN endpoint
    if (!(endpoint & 0x80)) {
        printf("[LPC55S69-Device] Invalid IN endpoint 0x%02x\n", endpoint);
        return -1;
    }
    
    uint8_t ep_num = endpoint & 0x0F;
    usb_status_t status = USB_DeviceSendRequest(device_handle, ep_num, buffer, length);
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Interrupt IN transfer failed with status %d\n", status);
        return -1;
    }
    
    return length;
}

int hurricane_hw_device_interrupt_out_transfer(
    uint8_t endpoint,
    void* buffer,
    uint16_t length)
{
    if (!device_initialized || !device_attached) {
        printf("[LPC55S69-Device] Device not initialized or not attached\n");
        return -1;
    }
    
    // Make sure this is an OUT endpoint
    if (endpoint & 0x80) {
        printf("[LPC55S69-Device] Invalid OUT endpoint 0x%02x\n", endpoint);
        return -1;
    }
    
    uint8_t ep_num = endpoint & 0x0F;
    usb_status_t status = USB_DeviceRecvRequest(device_handle, ep_num, buffer, length);
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Interrupt OUT transfer failed with status %d\n", status);
        return -1;
    }
    
    return length;
}

void hurricane_hw_device_set_configuration_callback(
    void (*callback)(uint8_t configuration))
{
    g_set_configuration_callback = callback;
}

void hurricane_hw_device_set_interface_callback(
    void (*callback)(uint8_t interface, uint8_t alt_setting))
{
    g_set_interface_callback = callback;
}

int hurricane_hw_device_configure_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol)
{
    if (interface_num >= USB_DEVICE_CONFIG_INTERFACES) {
        printf("[LPC55S69-Device] Error: Interface number %d exceeds maximum supported interfaces\n",
               interface_num);
        return -1;
    }
    
    // Store the configuration
    configured_interfaces[interface_num].configured = true;
    configured_interfaces[interface_num].interface_class = interface_class;
    configured_interfaces[interface_num].interface_subclass = interface_subclass;
    configured_interfaces[interface_num].interface_protocol = interface_protocol;
    
    // Rebuild the configuration descriptor to include this interface
    if (rebuild_configuration_descriptor() != 0) {
        printf("[LPC55S69-Device] Warning: Failed to rebuild configuration descriptor\n");
        // Continue anyway as the interface is still tracked
    }
    
    printf("[LPC55S69-Device] Configured interface %d (class %d, subclass %d, protocol %d)\n",
           interface_num, interface_class, interface_subclass, interface_protocol);
    
    return 0;
}

/**
 * @brief Rebuild the configuration descriptor based on configured interfaces
 *
 * This function creates a new configuration descriptor that includes all
 * dynamically added interfaces and their endpoints.
 *
 * @return 0 on success, negative error code on failure
 */
static int rebuild_configuration_descriptor(void)
{
    // Calculate the total size needed for the configuration descriptor
    // Start with the base configuration descriptor size (9 bytes)
    uint16_t total_size = 9;
    uint8_t num_interfaces = 0;
    
    // Count interfaces and calculate size
    for (int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
        if (configured_interfaces[i].configured) {
            num_interfaces++;
            // Add interface descriptor size (9 bytes)
            total_size += 9;
            
            // Add endpoint descriptors size (7 bytes each)
            total_size += configured_interfaces[i].num_endpoints * 7;
            
            // TODO: Add HID descriptor if needed
        }
    }
    
    // If no interfaces are configured, there's nothing to do
    if (num_interfaces == 0) {
        printf("[LPC55S69-Device] No interfaces configured, skipping descriptor rebuild\n");
        return 0;
    }
    
    // Allocate memory for the new configuration descriptor
    uint8_t* new_config_descriptor = malloc(total_size);
    if (!new_config_descriptor) {
        printf("[LPC55S69-Device] Error: Failed to allocate memory for config descriptor\n");
        return -1;
    }
    
    // Fill in the configuration descriptor header
    new_config_descriptor[0] = 9;                      // bLength
    new_config_descriptor[1] = USB_DESC_TYPE_CONFIGURATION;  // bDescriptorType
    new_config_descriptor[2] = total_size & 0xFF;      // wTotalLength (low byte)
    new_config_descriptor[3] = (total_size >> 8) & 0xFF; // wTotalLength (high byte)
    new_config_descriptor[4] = num_interfaces;         // bNumInterfaces
    new_config_descriptor[5] = 1;                      // bConfigurationValue
    new_config_descriptor[6] = 0;                      // iConfiguration (no string)
    new_config_descriptor[7] = 0x80;                   // bmAttributes (bus powered)
    new_config_descriptor[8] = 50;                     // bMaxPower (100mA)
    
    // Current position in the descriptor buffer for adding interface/endpoint descriptors
    uint16_t current_pos = 9;
    
    // Add interface and endpoint descriptors
    for (int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
        if (configured_interfaces[i].configured) {
            // Add interface descriptor
            new_config_descriptor[current_pos + 0] = 9;    // bLength
            new_config_descriptor[current_pos + 1] = USB_DESC_TYPE_INTERFACE; // bDescriptorType
            new_config_descriptor[current_pos + 2] = i;    // bInterfaceNumber
            new_config_descriptor[current_pos + 3] = 0;    // bAlternateSetting
            new_config_descriptor[current_pos + 4] = configured_interfaces[i].num_endpoints; // bNumEndpoints
            new_config_descriptor[current_pos + 5] = configured_interfaces[i].interface_class; // bInterfaceClass
            new_config_descriptor[current_pos + 6] = configured_interfaces[i].interface_subclass; // bInterfaceSubClass
            new_config_descriptor[current_pos + 7] = configured_interfaces[i].interface_protocol; // bInterfaceProtocol
            new_config_descriptor[current_pos + 8] = 0;    // iInterface (no string)
            
            current_pos += 9;
            
            // Add HID descriptor if this is a HID interface
            if (configured_interfaces[i].interface_class == 3) { // HID class
                // TODO: Add HID descriptor if needed
            }
            
            // Add endpoint descriptors
            for (int j = 0; j < configured_interfaces[i].num_endpoints; j++) {
                new_config_descriptor[current_pos + 0] = 7;    // bLength
                new_config_descriptor[current_pos + 1] = USB_DESC_TYPE_ENDPOINT; // bDescriptorType
                new_config_descriptor[current_pos + 2] = configured_interfaces[i].endpoints[j].ep_address; // bEndpointAddress
                new_config_descriptor[current_pos + 3] = configured_interfaces[i].endpoints[j].ep_attributes; // bmAttributes
                new_config_descriptor[current_pos + 4] = configured_interfaces[i].endpoints[j].ep_max_packet_size & 0xFF; // wMaxPacketSize (low byte)
                new_config_descriptor[current_pos + 5] = (configured_interfaces[i].endpoints[j].ep_max_packet_size >> 8) & 0xFF; // wMaxPacketSize (high byte)
                new_config_descriptor[current_pos + 6] = configured_interfaces[i].endpoints[j].ep_interval; // bInterval
                
                current_pos += 7;
            }
        }
    }
    
    // Free previous configuration descriptor if it exists
    if (g_config_descriptor) {
        free(g_config_descriptor);
    }
    
    // Update the global descriptor
    g_config_descriptor = new_config_descriptor;
    g_config_descriptor_length = total_size;
    
    printf("[LPC55S69-Device] Rebuilt configuration descriptor (%d bytes, %d interfaces)\n",
           total_size, num_interfaces);
           
    return 0;
}

int hurricane_hw_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval)
{
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return -1;
    }
    
    uint8_t ep_num = ep_address & 0x0F;
    uint8_t direction = (ep_address & 0x80) ? USB_IN : USB_OUT;
    usb_endpoint_type_t ep_type;
    
    // Convert Hurricane attributes to NXP USB type
    switch (ep_attributes & 0x03) {
        case 0:
            ep_type = USB_ENDPOINT_CONTROL;
            break;
        case 1:
            ep_type = USB_ENDPOINT_ISOCHRONOUS;
            break;
        case 2:
            ep_type = USB_ENDPOINT_BULK;
            break;
        case 3:
            ep_type = USB_ENDPOINT_INTERRUPT;
            break;
        default:
            ep_type = USB_ENDPOINT_CONTROL;
            break;
    }
    
    usb_device_endpoint_init_struct_t ep_init = {
        .zlt = 0,
        .transferType = ep_type,
        .endpointAddress = ep_address,
        .maxPacketSize = ep_max_packet_size,
        .interval = ep_interval
    };
    
    usb_device_endpoint_callback_struct_t ep_callback = {
        .callbackFn = NULL,  // We'll handle callbacks through the general device callback
        .callbackParam = NULL
    };
    
    usb_status_t status = USB_DeviceInitEndpoint(device_handle, &ep_init, &ep_callback);
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Failed to initialize endpoint 0x%02x with status %d\n",
               ep_address, status);
        return -1;
    }
    
    // Store endpoint configuration for dynamic descriptor building
    if (interface_num < USB_DEVICE_CONFIG_INTERFACES && configured_interfaces[interface_num].configured) {
        // Find an empty slot in the endpoints array
        uint8_t index = configured_interfaces[interface_num].num_endpoints;
        
        // Make sure we don't exceed the max number of endpoints per interface
        if (index < MAX_ENDPOINTS_PER_INTERFACE) {
            configured_interfaces[interface_num].endpoints[index].ep_address = ep_address;
            configured_interfaces[interface_num].endpoints[index].ep_attributes = ep_attributes;
            configured_interfaces[interface_num].endpoints[index].ep_max_packet_size = ep_max_packet_size;
            configured_interfaces[interface_num].endpoints[index].ep_interval = ep_interval;
            configured_interfaces[interface_num].num_endpoints++;
            
            // Rebuild the configuration descriptor to include this endpoint
            if (rebuild_configuration_descriptor() != 0) {
                printf("[LPC55S69-Device] Warning: Failed to rebuild configuration descriptor\n");
                // Continue anyway as the endpoint is still configured in hardware
            }
        } else {
            printf("[LPC55S69-Device] Warning: Too many endpoints for interface %d\n", interface_num);
        }
    }
    
    printf("[LPC55S69-Device] Configured endpoint 0x%02x for interface %d\n",
           ep_address, interface_num);
    
    return 0;
}

int hurricane_hw_device_set_descriptors(
    const uint8_t* device_desc,
    uint16_t device_desc_length,
    const uint8_t* config_desc,
    uint16_t config_desc_length)
{
    if (!device_desc || !config_desc || 
        device_desc_length == 0 || config_desc_length == 0) {
        printf("[LPC55S69-Device] Invalid descriptor parameters\n");
        return -1;
    }
    
    // Free previous descriptors if they exist
    if (g_device_descriptor) {
        free(g_device_descriptor);
    }
    
    if (g_config_descriptor) {
        free(g_config_descriptor);
    }
    
    // Allocate and copy new descriptors
    g_device_descriptor = malloc(device_desc_length);
    if (!g_device_descriptor) {
        printf("[LPC55S69-Device] Failed to allocate device descriptor memory\n");
        return -1;
    }
    
    g_config_descriptor = malloc(config_desc_length);
    if (!g_config_descriptor) {
        free(g_device_descriptor);
        g_device_descriptor = NULL;
        printf("[LPC55S69-Device] Failed to allocate config descriptor memory\n");
        return -1;
    }
    
    memcpy(g_device_descriptor, device_desc, device_desc_length);
    memcpy(g_config_descriptor, config_desc, config_desc_length);
    
    g_device_descriptor_length = device_desc_length;
    g_config_descriptor_length = config_desc_length;
    
    // If we have configured interfaces, rebuild the descriptor
    // This ensures any previously configured interfaces are included
    for (int i = 0; i < USB_DEVICE_CONFIG_INTERFACES; i++) {
        if (configured_interfaces[i].configured) {
            rebuild_configuration_descriptor();
            break;
        }
    }
    
    printf("[LPC55S69-Device] Device descriptors updated\n");
    
    return 0;
}

int hurricane_hw_device_set_hid_report_descriptor(
    const uint8_t* report_desc,
    uint16_t report_desc_length)
{
    if (!report_desc || report_desc_length == 0) {
        printf("[LPC55S69-Device] Invalid HID report descriptor parameters\n");
        return -1;
    }
    
    // Free previous descriptor if it exists
    if (g_hid_report_descriptor) {
        free(g_hid_report_descriptor);
    }
    
    // Allocate and copy new descriptor
    g_hid_report_descriptor = malloc(report_desc_length);
    if (!g_hid_report_descriptor) {
        printf("[LPC55S69-Device] Failed to allocate HID report descriptor memory\n");
        return -1;
    }
    
    memcpy(g_hid_report_descriptor, report_desc, report_desc_length);
    g_hid_report_descriptor_length = report_desc_length;
    
    printf("[LPC55S69-Device] HID report descriptor updated (%d bytes)\n", 
           report_desc_length);
    
    return 0;
}

/**
 * @brief Set string descriptor for device mode
 */
int hurricane_hw_device_set_string_descriptor(
    uint8_t index,
    const uint8_t* str_desc,
    uint16_t str_desc_length)
{
    if (index >= USB_DEVICE_CONFIG_STRING_COUNT) {
        printf("[LPC55S69-Device] Error: String descriptor index %d exceeds maximum\n", index);
        return -1;
    }

    if (!str_desc || str_desc_length == 0) {
        printf("[LPC55S69-Device] Error: Invalid string descriptor parameters\n");
        return -1;
    }

    // Free previous descriptor if it exists
    if (g_string_descriptors[index]) {
        free(g_string_descriptors[index]);
        g_string_descriptors[index] = NULL;
        g_string_descriptor_lengths[index] = 0;
    }

    // Allocate memory for the string descriptor
    uint8_t* descriptor = malloc(str_desc_length);
    if (!descriptor) {
        printf("[LPC55S69-Device] Error: Failed to allocate string descriptor memory\n");
        return -1;
    }

    // Copy the descriptor
    memcpy(descriptor, str_desc, str_desc_length);

    // Store the descriptor in the device context
    // used by the USB stack when handling GET_DESCRIPTOR requests for string descriptors
    g_string_descriptors[index] = descriptor;
    g_string_descriptor_lengths[index] = str_desc_length;
    
    printf("[LPC55S69-Device] String descriptor %d updated (%d bytes)\n",
           index, str_desc_length);
    
    return 0;
}

/**
 * @brief Enable or disable a configured endpoint
 */
int hurricane_hw_device_endpoint_enable(
    uint8_t ep_address,
    bool enable)
{
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return -1;
    }

    uint8_t ep_num = ep_address & 0x0F;
    uint8_t direction = (ep_address & 0x80) ? USB_IN : USB_OUT;
    usb_status_t status = kStatus_USB_Success;
    
    if (enable) {
        // Enable endpoint - LPC55S69 specific implementation
        // For IP3511 controller, we use different APIs than RT1060
        status = USB_DeviceLpc3511IpEndpointEnable(device_handle, ep_address, USB_ENDPOINT_INTERRUPT, ep_num);
    } else {
        // Disable endpoint
        status = USB_DeviceLpc3511IpEndpointDisable(device_handle, ep_address);
    }
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Failed to %s endpoint 0x%02x with status %d\n",
               enable ? "enable" : "disable", ep_address, status);
        return -1;
    }
    
    printf("[LPC55S69-Device] Endpoint 0x%02x %s\n",
           ep_address, enable ? "enabled" : "disabled");
    
    return 0;
}

/**
 * @brief Stall or unstall an endpoint
 */
int hurricane_hw_device_endpoint_stall(
    uint8_t ep_address,
    bool stall)
{
    if (!device_initialized) {
        printf("[LPC55S69-Device] Device not initialized\n");
        return -1;
    }

    usb_status_t status;
    
    if (stall) {
        // Stall endpoint
        status = USB_DeviceStallEndpoint(device_handle, ep_address);
    } else {
        // Unstall endpoint
        status = USB_DeviceUnstallEndpoint(device_handle, ep_address);
    }
    
    if (status != kStatus_USB_Success) {
        printf("[LPC55S69-Device] Failed to %s endpoint 0x%02x with status %d\n",
               stall ? "stall" : "unstall", ep_address, status);
        return -1;
    }
    
    printf("[LPC55S69-Device] Endpoint 0x%02x %s\n",
           ep_address, stall ? "stalled" : "unstalled");
    
    return 0;
}

//==============================================================================
// Private functions
//==============================================================================

static usb_status_t USB_DeviceCallback(usb_device_handle handle, 
                                      uint32_t event, 
                                      void *param)
{
    usb_status_t status = kStatus_USB_Success;
    
    switch (event) {
        case kUSB_DeviceEventBusReset:
            device_attached = true;
            current_configuration = 0;
            printf("[LPC55S69-Device] USB bus reset\n");
            break;
            
        case kUSB_DeviceEventSetConfiguration:
            current_configuration = *(uint8_t *)param;
            printf("[LPC55S69-Device] Set configuration %d\n", current_configuration);
            
            // Call user callback if registered
            if (g_set_configuration_callback) {
                g_set_configuration_callback(current_configuration);
            }
            break;
            
        case kUSB_DeviceEventSetInterface:
            // For simplicity, casting to 16-bit with interface in low byte, setting in high byte
            current_interface = ((*(uint16_t *)param) & 0xFF);
            current_alternate_setting = ((*(uint16_t *)param) >> 8);
            printf("[LPC55S69-Device] Set interface %d alternate setting %d\n", 
                   current_interface, current_alternate_setting);
                   
            // Call user callback if registered
            if (g_set_interface_callback) {
                g_set_interface_callback(current_interface, current_alternate_setting);
            }
            break;
            
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (g_device_descriptor && g_device_descriptor_length > 0) {
                usb_device_get_device_descriptor_struct_t *device_descriptor = 
                    (usb_device_get_device_descriptor_struct_t *)param;
                    
                // Return the custom device descriptor
                device_descriptor->buffer = g_device_descriptor;
                device_descriptor->length = g_device_descriptor_length;
            } else {
                // If no custom descriptor is set, return an error
                status = kStatus_USB_InvalidRequest;
            }
            break;
            
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (g_config_descriptor && g_config_descriptor_length > 0) {
                usb_device_get_configuration_descriptor_struct_t *config_descriptor =
                    (usb_device_get_configuration_descriptor_struct_t *)param;
                    
                // Return the dynamically built config descriptor
                config_descriptor->buffer = g_config_descriptor;
                config_descriptor->length = g_config_descriptor_length;
                
                printf("[LPC55S69-Device] Providing configuration descriptor (%d bytes)\n",
                       g_config_descriptor_length);
            } else {
                // If no custom descriptor is set, return an error
                status = kStatus_USB_InvalidRequest;
                printf("[LPC55S69-Device] Error: No configuration descriptor available\n");
            }
            break;
            
        case kUSB_DeviceEventGetHidReportDescriptor:
            if (g_hid_report_descriptor && g_hid_report_descriptor_length > 0) {
                usb_device_get_hid_report_descriptor_struct_t *hid_report_descriptor = 
                    (usb_device_get_hid_report_descriptor_struct_t *)param;
                    
                // Return the custom HID report descriptor
                hid_report_descriptor->buffer = g_hid_report_descriptor;
                hid_report_descriptor->length = g_hid_report_descriptor_length;
            } else {
                // If no HID report descriptor is set, return an error
                status = kStatus_USB_InvalidRequest;
            }
            break;
            
        case kUSB_DeviceEventGetStringDescriptor:
            {
                usb_device_get_string_descriptor_struct_t *string_descriptor =
                    (usb_device_get_string_descriptor_struct_t *)param;
                
                // Extract string index from the request
                uint8_t string_index = (uint8_t)(string_descriptor->stringIndex);
                
                // Check if we have this string descriptor
                if (string_index < USB_DEVICE_CONFIG_STRING_COUNT &&
                    g_string_descriptors[string_index] &&
                    g_string_descriptor_lengths[string_index] > 0) {
                    
                    // Return the stored string descriptor
                    string_descriptor->buffer = g_string_descriptors[string_index];
                    string_descriptor->length = g_string_descriptor_lengths[string_index];
                    
                    printf("[LPC55S69-Device] Providing string descriptor %d (%d bytes)\n",
                           string_index, g_string_descriptor_lengths[string_index]);
                } else {
                    // If no custom descriptor is set, return an error
                    status = kStatus_USB_InvalidRequest;
                    printf("[LPC55S69-Device] Error: String descriptor %d not available\n", string_index);
                }
            }
            break;
            
        case kUSB_DeviceEventSetup:
            // Forward setup packets to the setup packet handler
            status = USB_DeviceSetupPacketHandler(handle,
                                                (usb_setup_struct_t *)param,
                                                NULL, NULL);
            break;
            
        default:
            // Unhandled event
            break;
    }
    
    return status;
}

/**
 * @brief Response callback for control requests
 *
 * This function is called by interface handlers to provide response data
 * for control requests.
 *
 * @param interface_num Interface that is responding
 * @param handled True if request was handled, false otherwise
 * @param buffer Buffer containing response data (can be NULL)
 * @param length Length of response data
 */
static void control_request_response_callback(uint8_t interface_num, bool handled, void* buffer, uint16_t length) {
    if (!control_state.response_pending) {
        printf("[LPC55S69-Device] Warning: Control response received without pending request\n");
        return;
    }

    // Store response data
    control_state.handled = handled;
    control_state.request_info.interface_num = interface_num;
    
    if (handled && buffer != NULL && length > 0) {
        // Copy response data to our buffer
        if (length > sizeof(control_state.buffer)) {
            printf("[LPC55S69-Device] Warning: Control response truncated (%u > %u)\n",
                   length, sizeof(control_state.buffer));
            length = sizeof(control_state.buffer);
        }
        memcpy(control_state.buffer, buffer, length);
    }
    
    control_state.length = length;
    control_state.response_ready = true;
    
    printf("[LPC55S69-Device] Control response received from interface %d, handled=%d, length=%u\n",
           interface_num, handled, length);
}

/**
 * @brief Complete a control transfer based on response
 *
 * @param setup Setup packet for the control request
 * @param buffer Buffer containing data for response (for IN transfers)
 * @param length Length of data
 * @return kStatus_USB_Success if successful, otherwise error code
 */
static usb_status_t complete_control_transfer(
    usb_setup_struct_t *setup,
    uint8_t *buffer,
    uint32_t length)
{
    usb_status_t status = kStatus_USB_Success;
    
    // For device-to-host control transfers, we need to send the data
    if (setup->bmRequestType & 0x80) { // Device-to-host (IN)
        if (length > 0 && buffer != NULL) {
            status = USB_DeviceSendRequest(device_handle, 0, buffer, length);
            if (status != kStatus_USB_Success) {
                printf("[LPC55S69-Device] Failed to send control response, status=%d\n", status);
            }
        } else {
            // Zero-length packet for acknowledge
            status = USB_DeviceSendRequest(device_handle, 0, NULL, 0);
        }
    } else { // Host-to-device (OUT)
        // For host-to-device, we need to receive the data if any
        if (setup->wLength > 0) {
            status = USB_DeviceRecvRequest(device_handle, 0, buffer, setup->wLength);
            if (status != kStatus_USB_Success) {
                printf("[LPC55S69-Device] Failed to receive control data, status=%d\n", status);
            }
        } else {
            // Send zero-length packet as acknowledgement
            status = USB_DeviceSendRequest(device_handle, 0, NULL, 0);
        }
    }
    
    return status;
}


/**
 * @brief Check if a control request timeout has occurred
 *
 * @return true if timeout has occurred, false otherwise
 */
static bool is_control_request_timeout(void) {
    static uint32_t timeout_counter = 0;
    static const uint32_t TIMEOUT_THRESHOLD = 1000; // Adjust as needed
    
    timeout_counter++;
    
    if (timeout_counter > TIMEOUT_THRESHOLD) {
        timeout_counter = 0;
        return true;
    }
    
    return false;
}

static usb_status_t USB_DeviceSetupPacketHandler(usb_device_handle handle,
                                               usb_setup_struct_t *setup,
                                               uint32_t *length,
                                               uint8_t **buffer)
{
    // Convert NXP setup packet to Hurricane setup packet
    hurricane_usb_setup_packet_t hurricane_setup;
    hurricane_setup.bmRequestType = setup->bmRequestType;
    hurricane_setup.bRequest = setup->bRequest;
    hurricane_setup.wValue = setup->wValue;
    hurricane_setup.wIndex = setup->wIndex;
    hurricane_setup.wLength = setup->wLength;
    
    // We'll need to handle standard requests internally
    // but for class and vendor requests, we can forward to the interface manager
    
    // Standard request handling first
    if ((setup->bmRequestType & USB_REQUEST_TYPE_TYPE_MASK) ==
        USB_REQUEST_TYPE_STANDARD) {
        // Most standard requests are already handled by the NXP stack
        // We only need to handle additional ones or override behavior
        
        switch (setup->bRequest) {
            case USB_REQUEST_STANDARD_GET_DESCRIPTOR:
                // Check if this is a string descriptor request
                if ((setup->wValue >> 8) == USB_DESCRIPTOR_TYPE_STRING) {
                    uint8_t string_index = setup->wValue & 0xFF;
                    
                    // Check if we have this string descriptor
                    if (string_index < USB_DEVICE_CONFIG_STRING_COUNT &&
                        g_string_descriptors[string_index] &&
                        g_string_descriptor_lengths[string_index] > 0) {
                        
                        // Create a new buffer for the response if needed
                        if (buffer != NULL && length != NULL) {
                            *buffer = g_string_descriptors[string_index];
                            *length = g_string_descriptor_lengths[string_index];
                            
                            printf("[LPC55S69-Device] GET_DESCRIPTOR: Providing string %d (%d bytes)\n",
                                  string_index, g_string_descriptor_lengths[string_index]);
                        }
                        return kStatus_USB_Success;
                    } else {
                        printf("[LPC55S69-Device] GET_DESCRIPTOR: String %d not available\n", string_index);
                        return kStatus_USB_InvalidRequest;
                    }
                }
                // Most other descriptor requests are handled by the callbacks above
                break;
                
            case USB_REQUEST_STANDARD_SET_ADDRESS:
                // Address is handled by the USB controller hardware
                break;
                
            default:
                // Other standard requests
                break;
        }
        
        return kStatus_USB_Success;
    }
    
    // For class and vendor requests, notify the interface manager and wait for a response
    
    // Determine the interface from wIndex for interface requests
    uint8_t interface_num = 0;
    if ((setup->bmRequestType & USB_REQUEST_TYPE_RECIPIENT_MASK) ==
        USB_REQUEST_TYPE_RECIPIENT_INTERFACE) {
        interface_num = setup->wIndex & 0xFF;
    }
    
    // Initialize control request state
    memset(&control_state, 0, sizeof(control_state));
    control_state.response_pending = true;
    control_state.request_info.interface_num = interface_num;
    control_state.request_info.setup = hurricane_setup;
    
    printf("[LPC55S69-Device] Forwarding class/vendor control request to interface %d\n", interface_num);
    
    // Notify the interface manager of the control request with our response callback
    bool handled = hurricane_interface_notify_event_with_response(
        USB_EVENT_CONTROL_REQUEST,
        interface_num,
        &hurricane_setup,
        control_request_response_callback);
    
    // If the request wasn't handled synchronously, wait for the response
    if (!handled && control_state.response_pending) {
        // Wait for a response or timeout
        // In a real implementation, we might use a semaphore or other synchronization primitive
        bool timeout = false;
        
        printf("[LPC55S69-Device] Waiting for control request response...\n");
        
        while (!control_state.response_ready && !timeout) {
            // Check for timeout
            if (is_control_request_timeout()) {
                timeout = true;
                break;
            }
            
            // Give other threads a chance to run
            // In a real implementation with an RTOS, we would use a semaphore and/or sleep
            // For polling implementations, we need to allow USB events to be processed
            for (volatile int i = 0; i < 1000; i++) {
                __asm("nop"); // Reduce CPU usage while busy-waiting
            }
            
            // Continue processing USB events
            // Note: The specific task function depends on the USB controller type
            // For LPC55S69 with IP3511FS controller, we don't call the task function directly
            // Instead we rely on interrupts and the hurricane_hw_device_poll function
            hurricane_hw_device_poll();
        }
        
        if (timeout) {
            printf("[LPC55S69-Device] Control request timeout\n");
            control_state.response_pending = false;
            return kStatus_USB_InvalidRequest;
        }
    }
    
    // Check if the request was handled
    if (!control_state.handled) {
        printf("[LPC55S69-Device] Control request not handled\n");
        control_state.response_pending = false;
        return kStatus_USB_InvalidRequest;
    }
    
    // Provide response data if available
    if (buffer != NULL && length != NULL && control_state.length > 0) {
        // For IN transfers (device to host), provide the data buffer
        if (setup->bmRequestType & 0x80) {
            *buffer = control_state.buffer;
            *length = control_state.length;
        }
        
                printf("[LPC55S69-Device] Providing control response (%u bytes)\n", control_state.length);
            }
            
            // Complete the transfer with the buffer and length
            usb_status_t transfer_status = complete_control_transfer(setup, *buffer, *length);
            if (transfer_status != kStatus_USB_Success) {
                printf("[LPC55S69-Device] Control transfer completion failed: %d\n", transfer_status);
            }
        } else {
            // For requests without data, or unhandled requests, send a status response
            if (control_state.handled) {
                complete_control_transfer(setup, NULL, 0);
            }
        }
        
        control_state.response_pending = false;
        return control_state.handled ? kStatus_USB_Success : kStatus_USB_InvalidRequest;
}
