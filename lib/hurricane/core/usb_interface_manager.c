/**
 * @file usb_interface_manager.c
 * @brief Implementation of the dynamic USB interface configuration and management
 */

#include "usb_interface_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "hw/hurricane_hw_hal.h"

// Private definitions
#define MAX_HOST_CLASS_HANDLERS 8
#define MAX_INTERFACE_REGISTRY_ENTRIES 16

// Define error codes
#define HURRICANE_ERROR_NONE 0
#define HURRICANE_ERROR_INVALID_PARAM -1
#define HURRICANE_ERROR_NO_MEMORY -2
#define HURRICANE_ERROR_NOT_FOUND -3
#define HURRICANE_ERROR_ALREADY_EXISTS -4

// Mutex for thread safety
static pthread_mutex_t interface_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

// Registry for device-mode interfaces
static hurricane_interface_registry_entry_t* device_interface_registry = NULL;

// Registry for host-mode class handlers
typedef struct {
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    hurricane_host_class_handler_t handler;
    bool active;
} host_class_handler_entry_t;

static host_class_handler_entry_t host_class_handlers[MAX_HOST_CLASS_HANDLERS];
static uint8_t num_host_class_handlers = 0;

// Current device descriptors
static hurricane_device_descriptors_t current_device_descriptors = {0};

// Forward declarations of helper functions
static hurricane_interface_registry_entry_t* find_device_interface(uint8_t interface_num);
static hurricane_endpoint_descriptor_t* find_device_endpoint(hurricane_interface_registry_entry_t* interface, uint8_t ep_address);
static host_class_handler_entry_t* find_host_class_handler(uint8_t device_class, uint8_t device_subclass, uint8_t device_protocol);

void hurricane_interface_manager_init(void)
{
    pthread_mutex_lock(&interface_manager_mutex);
    
    // Initialize device interface registry
    device_interface_registry = NULL;
    
    // Initialize host class handlers
    memset(host_class_handlers, 0, sizeof(host_class_handlers));
    num_host_class_handlers = 0;
    
    // Initialize device descriptors
    memset(&current_device_descriptors, 0, sizeof(current_device_descriptors));
    
    pthread_mutex_unlock(&interface_manager_mutex);
    
    printf("[Interface Manager] Initialized\n");
}

/**
 * @brief Deinitialize the interface manager
 *
 * This function cleans up all resources allocated by the interface manager.
 * It should be called during system shutdown to prevent memory leaks.
 */
void hurricane_interface_manager_deinit(void)
{
    pthread_mutex_lock(&interface_manager_mutex);
    
    // Free all interface registry entries
    hurricane_interface_registry_entry_t* current = device_interface_registry;
    hurricane_interface_registry_entry_t* next = NULL;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    device_interface_registry = NULL;
    
    // Free device descriptors if they exist
    if (current_device_descriptors.device_descriptor) {
        free(current_device_descriptors.device_descriptor);
    }
    
    if (current_device_descriptors.config_descriptor) {
        free(current_device_descriptors.config_descriptor);
    }
    
    if (current_device_descriptors.hid_report_descriptor) {
        free(current_device_descriptors.hid_report_descriptor);
    }
    
    for (int i = 0; i < MAX_STRING_DESCRIPTORS; i++) {
        if (current_device_descriptors.string_descriptors[i]) {
            free(current_device_descriptors.string_descriptors[i]);
            current_device_descriptors.string_descriptors[i] = NULL;
        }
    }
    
    memset(&current_device_descriptors, 0, sizeof(current_device_descriptors));
    
    pthread_mutex_unlock(&interface_manager_mutex);
    
    printf("[Interface Manager] Deinitialized and resources freed\n");
}

int hurricane_add_device_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol,
    const hurricane_interface_descriptor_t* descriptor)
{
    pthread_mutex_lock(&interface_manager_mutex);
    if (!descriptor) {
        printf("[Interface Manager] Error: Null descriptor\n");
        return HURRICANE_ERROR_INVALID_PARAM;
    }
    
    // Check if interface already exists
    hurricane_interface_registry_entry_t* existing = find_device_interface(interface_num);
    if (existing) {
        printf("[Interface Manager] Error: Interface %d already exists\n", interface_num);
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_ALREADY_EXISTS;
    }
    
    // Create new interface registry entry
    hurricane_interface_registry_entry_t* new_entry = 
        (hurricane_interface_registry_entry_t*)malloc(sizeof(hurricane_interface_registry_entry_t));
    
    if (!new_entry) {
        printf("[Interface Manager] Error: Failed to allocate memory for interface %d\n", interface_num);
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_NO_MEMORY;
    }
    
    // Copy descriptor data
    memcpy(&new_entry->descriptor, descriptor, sizeof(hurricane_interface_descriptor_t));
    memset(new_entry->endpoints, 0, sizeof(new_entry->endpoints));
    new_entry->active = true;
    new_entry->next = NULL;
    
    // Add to list (at head for simplicity)
    if (device_interface_registry == NULL) {
        device_interface_registry = new_entry;
    } else {
        new_entry->next = device_interface_registry;
        device_interface_registry = new_entry;
    }
    
    // Configure the interface at hardware level
    int result = hurricane_hw_device_configure_interface(
        interface_num, interface_class, interface_subclass, interface_protocol);
    
    if (result != 0) {
        printf("[Interface Manager] Warning: Hardware interface configuration returned %d\n", result);
        // We continue anyway as this might be a soft error
    }
    
    printf("[Interface Manager] Added interface %d (class %d, subclass %d, protocol %d)\n", 
           interface_num, interface_class, interface_subclass, interface_protocol);
           
    // Notify event listeners
    hurricane_interface_notify_event(USB_EVENT_INTERFACE_ENABLED, interface_num, NULL);
    
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

int hurricane_remove_device_interface(uint8_t interface_num)
{
    pthread_mutex_lock(&interface_manager_mutex);
    hurricane_interface_registry_entry_t* current = device_interface_registry;
    hurricane_interface_registry_entry_t* prev = NULL;
    
    // Find the interface in the registry
    while (current != NULL) {
        if (current->descriptor.interface_num == interface_num) {
            // Found it, remove from list
            if (prev == NULL) {
                // It's the head of the list
                device_interface_registry = current->next;
            } else {
                // It's in the middle/end of the list
                prev->next = current->next;
            }
            
            // Notify event listeners before freeing
            hurricane_interface_notify_event(USB_EVENT_INTERFACE_DISABLED, interface_num, NULL);
            
            // Free the memory
            free(current);
            
            printf("[Interface Manager] Removed interface %d\n", interface_num);
            return HURRICANE_ERROR_NONE;
        }
        
        prev = current;
        current = current->next;
    }
    
    printf("[Interface Manager] Error: Interface %d not found\n", interface_num);
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NOT_FOUND;
}

int hurricane_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval)
{
    pthread_mutex_lock(&interface_manager_mutex);
    // Find the interface
    hurricane_interface_registry_entry_t* interface = find_device_interface(interface_num);
    if (!interface) {
        printf("[Interface Manager] Error: Interface %d not found\n", interface_num);
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_NOT_FOUND;
    }
    
    // Find existing endpoint or empty slot
    hurricane_endpoint_descriptor_t* endpoint = find_device_endpoint(interface, ep_address);
    if (!endpoint) {
        // Find an empty slot
        uint8_t i;
        for (i = 0; i < MAX_ENDPOINTS_PER_INTERFACE; i++) {
            if (!interface->endpoints[i].configured) {
                endpoint = &interface->endpoints[i];
                break;
            }
        }
        
        if (!endpoint) {
            printf("[Interface Manager] Error: No available endpoint slots for interface %d\n",
                   interface_num);
            pthread_mutex_unlock(&interface_manager_mutex);
            return HURRICANE_ERROR_NO_MEMORY;
        }
    }
    
    // Configure the endpoint
    endpoint->ep_address = ep_address;
    endpoint->ep_attributes = ep_attributes;
    endpoint->ep_max_packet_size = ep_max_packet_size;
    endpoint->ep_interval = ep_interval;
    endpoint->configured = true;
    
    // Configure the endpoint at hardware level
    int result = hurricane_hw_device_configure_endpoint(
        interface_num, ep_address, ep_attributes, ep_max_packet_size, ep_interval);
    
    if (result != 0) {
        printf("[Interface Manager] Warning: Hardware endpoint configuration returned %d\n", result);
        // We continue anyway as this might be a soft error
    }
    
    printf("[Interface Manager] Configured endpoint 0x%02X for interface %d\n",
           ep_address, interface_num);
    
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

void hurricane_device_interface_register_control_handler(
    uint8_t interface_num,
    bool (*handler)(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length))
{
    pthread_mutex_lock(&interface_manager_mutex);
    hurricane_interface_registry_entry_t* interface = find_device_interface(interface_num);
    if (!interface) {
        printf("[Interface Manager] Error: Interface %d not found for control handler\n", interface_num);
        pthread_mutex_unlock(&interface_manager_mutex);
        return;
    }
    
    interface->descriptor.control_handler = handler;
    printf("[Interface Manager] Registered control handler for interface %d\n", interface_num);
    pthread_mutex_unlock(&interface_manager_mutex);
}

int hurricane_register_host_class_handler(
    uint8_t device_class,
    uint8_t device_subclass,
    uint8_t device_protocol,
    const hurricane_host_class_handler_t* handler)
{
    pthread_mutex_lock(&interface_manager_mutex);
    if (!handler) {
        printf("[Interface Manager] Error: Null handler\n");
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_INVALID_PARAM;
    }
    
    // Check if handler already exists
    host_class_handler_entry_t* existing = 
        find_host_class_handler(device_class, device_subclass, device_protocol);
    
    if (existing) {
        printf("[Interface Manager] Error: Handler for class %d, subclass %d, protocol %d already exists\n",
               device_class, device_subclass, device_protocol);
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_ALREADY_EXISTS;
    }
    
    // Check if we have room
    if (num_host_class_handlers >= MAX_HOST_CLASS_HANDLERS) {
        printf("[Interface Manager] Error: Host class handler registry full\n");
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_NO_MEMORY;
    }
    
    // Add new handler
    host_class_handler_entry_t* new_handler = &host_class_handlers[num_host_class_handlers];
    new_handler->device_class = device_class;
    new_handler->device_subclass = device_subclass;
    new_handler->device_protocol = device_protocol;
    memcpy(&new_handler->handler, handler, sizeof(hurricane_host_class_handler_t));
    new_handler->active = true;
    
    num_host_class_handlers++;
    
    printf("[Interface Manager] Registered host handler for class %d, subclass %d, protocol %d\n",
           device_class, device_subclass, device_protocol);
    
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

int hurricane_unregister_host_class_handler(
    uint8_t device_class,
    uint8_t device_subclass,
    uint8_t device_protocol)
{
    pthread_mutex_lock(&interface_manager_mutex);
    // Find the handler
    host_class_handler_entry_t* handler = 
        find_host_class_handler(device_class, device_subclass, device_protocol);
    
    if (!handler) {
        printf("[Interface Manager] Error: Handler for class %d, subclass %d, protocol %d not found\n",
               device_class, device_subclass, device_protocol);
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_NOT_FOUND;
    }
    
    // Mark as inactive (we don't actually remove it to avoid shifting the array)
    handler->active = false;
    
    printf("[Interface Manager] Unregistered host handler for class %d, subclass %d, protocol %d\n",
           device_class, device_subclass, device_protocol);
    
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

void hurricane_interface_notify_event(
    hurricane_usb_event_t event,
    uint8_t interface_num,
    void* event_data)
{
    pthread_mutex_lock(&interface_manager_mutex);
    printf("[Interface Manager] Event %d on interface %d\n", event, interface_num);
    
    // For device events
    if (event == USB_EVENT_CONTROL_REQUEST && event_data) {
        hurricane_interface_registry_entry_t* interface = find_device_interface(interface_num);
        if (interface && interface->descriptor.control_handler) {
            hurricane_usb_setup_packet_t* setup = (hurricane_usb_setup_packet_t*)event_data;
            uint16_t length = setup->wLength;
            
            // Call the control handler
            bool handled = interface->descriptor.control_handler(setup, NULL, &length);
            
            printf("[Interface Manager] Control request %s by interface %d handler\n", 
                   handled ? "handled" : "not handled", interface_num);
        }
    }
    
    // For host events
    if (event == USB_EVENT_DEVICE_ATTACHED && event_data) {
        // Find matching handler
        void* device = event_data;
        
        // In a real implementation, we would extract class/subclass/protocol from the device
        // For now, we'll just assume it's any HID device (class 3)
        uint8_t device_class = 3;  // HID
        uint8_t device_subclass = 0;
        uint8_t device_protocol = 0;
        
        host_class_handler_entry_t* handler = 
            find_host_class_handler(device_class, device_subclass, device_protocol);
        
        if (handler && handler->handler.attach_callback) {
            handler->handler.attach_callback(device);
            printf("[Interface Manager] Host handler attached device\n");
        }
    }
    
    if (event == USB_EVENT_DEVICE_DETACHED && event_data) {
        // Similar to attached, but for detach
        void* device = event_data;
        uint8_t device_class = 3;  // HID
        uint8_t device_subclass = 0;
        uint8_t device_protocol = 0;
        
        host_class_handler_entry_t* handler = 
            find_host_class_handler(device_class, device_subclass, device_protocol);
        
        if (handler && handler->handler.detach_callback) {
            handler->handler.detach_callback(device);
            printf("[Interface Manager] Host handler detached device\n");
        }
    }
    
    pthread_mutex_unlock(&interface_manager_mutex);
}

int hurricane_device_update_descriptors(hurricane_device_descriptors_t* descriptors)
{
    pthread_mutex_lock(&interface_manager_mutex);
    if (!descriptors) {
        printf("[Interface Manager] Error: Null device descriptors\n");
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_INVALID_PARAM;
    }
    
    // Copy the descriptor data
    memcpy(&current_device_descriptors, descriptors, sizeof(hurricane_device_descriptors_t));
    
    // Update the device stack with these descriptors
    if (descriptors->device_descriptor && descriptors->device_descriptor_length > 0 &&
        descriptors->config_descriptor && descriptors->config_descriptor_length > 0) {
        
        int result = hurricane_hw_device_set_descriptors(
            descriptors->device_descriptor,
            descriptors->device_descriptor_length,
            descriptors->config_descriptor,
            descriptors->config_descriptor_length);
            
        if (result != 0) {
            printf("[Interface Manager] Warning: Hardware descriptor update returned %d\n", result);
        }
    }
    
    // Update HID report descriptor if present
    if (descriptors->hid_report_descriptor && descriptors->hid_report_descriptor_length > 0) {
        int result = hurricane_hw_device_set_hid_report_descriptor(
            descriptors->hid_report_descriptor,
            descriptors->hid_report_descriptor_length);
            
        if (result != 0) {
            printf("[Interface Manager] Warning: Hardware HID report descriptor update returned %d\n", result);
        }
    }
    
    printf("[Interface Manager] Updated device descriptors\n");
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

int hurricane_device_update_report_descriptor(uint8_t* report_descriptor, uint16_t length)
{
    pthread_mutex_lock(&interface_manager_mutex);
    if (!report_descriptor || length == 0) {
        printf("[Interface Manager] Error: Invalid report descriptor\n");
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_INVALID_PARAM;
    }
    
    // Free old descriptor if it exists
    if (current_device_descriptors.hid_report_descriptor) {
        free(current_device_descriptors.hid_report_descriptor);
        current_device_descriptors.hid_report_descriptor = NULL;
        current_device_descriptors.hid_report_descriptor_length = 0;
    }
    
    // Allocate and copy new descriptor
    current_device_descriptors.hid_report_descriptor = malloc(length);
    if (!current_device_descriptors.hid_report_descriptor) {
        printf("[Interface Manager] Error: Failed to allocate memory for report descriptor\n");
        pthread_mutex_unlock(&interface_manager_mutex);
        return HURRICANE_ERROR_NO_MEMORY;
    }
    
    memcpy(current_device_descriptors.hid_report_descriptor, report_descriptor, length);
    current_device_descriptors.hid_report_descriptor_length = length;
    
    // Update the hardware with the new descriptor
    int result = hurricane_hw_device_set_hid_report_descriptor(report_descriptor, length);
    if (result != 0) {
        printf("[Interface Manager] Warning: Hardware HID report descriptor update returned %d\n", result);
    }
    
    printf("[Interface Manager] Updated HID report descriptor (%d bytes)\n", length);
    pthread_mutex_unlock(&interface_manager_mutex);
    return HURRICANE_ERROR_NONE;
}

int hurricane_device_trigger_reset(void)
{
    printf("[Interface Manager] Triggering USB device reset\n");
    
    // Call the hardware implementation to do the actual reset
    hurricane_hw_device_reset();
    
    return HURRICANE_ERROR_NONE;
}

const hurricane_interface_descriptor_t* hurricane_get_device_interface(uint8_t interface_num)
{
    pthread_mutex_lock(&interface_manager_mutex);
    hurricane_interface_registry_entry_t* interface = find_device_interface(interface_num);
    if (!interface) {
        pthread_mutex_unlock(&interface_manager_mutex);
        return NULL;
    }
    
    const hurricane_interface_descriptor_t* result = &interface->descriptor;
    pthread_mutex_unlock(&interface_manager_mutex);
    return result;
}

const hurricane_endpoint_descriptor_t* hurricane_get_device_endpoint(
    uint8_t interface_num,
    uint8_t ep_address)
{
    pthread_mutex_lock(&interface_manager_mutex);
    hurricane_interface_registry_entry_t* interface = find_device_interface(interface_num);
    if (!interface) {
        pthread_mutex_unlock(&interface_manager_mutex);
        return NULL;
    }
    
    const hurricane_endpoint_descriptor_t* result = find_device_endpoint(interface, ep_address);
    pthread_mutex_unlock(&interface_manager_mutex);
    return result;
}

// Helper functions

static hurricane_interface_registry_entry_t* find_device_interface(uint8_t interface_num)
{
    hurricane_interface_registry_entry_t* current = device_interface_registry;
    
    while (current != NULL) {
        if (current->descriptor.interface_num == interface_num && current->active) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

static hurricane_endpoint_descriptor_t* find_device_endpoint(
    hurricane_interface_registry_entry_t* interface, 
    uint8_t ep_address)
{
    if (!interface) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_ENDPOINTS_PER_INTERFACE; i++) {
        if (interface->endpoints[i].configured && 
            interface->endpoints[i].ep_address == ep_address) {
            return &interface->endpoints[i];
        }
    }
    
    return NULL;
}

static host_class_handler_entry_t* find_host_class_handler(
    uint8_t device_class, 
    uint8_t device_subclass, 
    uint8_t device_protocol)
{
    // We do a two-pass search:
    // 1. First look for exact match (class, subclass, protocol)
    // 2. Then look for class match with wildcards for subclass/protocol
    
    // First pass: exact match
    for (int i = 0; i < num_host_class_handlers; i++) {
        if (host_class_handlers[i].active && 
            host_class_handlers[i].device_class == device_class &&
            host_class_handlers[i].device_subclass == device_subclass &&
            host_class_handlers[i].device_protocol == device_protocol) {
            return &host_class_handlers[i];
        }
    }
    
    // Second pass: class match with wildcards
    for (int i = 0; i < num_host_class_handlers; i++) {
        if (host_class_handlers[i].active && 
            host_class_handlers[i].device_class == device_class &&
            (host_class_handlers[i].device_subclass == 0 ||  // 0 means any subclass
             host_class_handlers[i].device_subclass == device_subclass) &&
            (host_class_handlers[i].device_protocol == 0 ||  // 0 means any protocol 
             host_class_handlers[i].device_protocol == device_protocol)) {
            return &host_class_handlers[i];
        }
    }
    
    return NULL;
}
