/**
 * @file usb_interface_manager.h
 * @brief Dynamic USB interface configuration and management
 *
 * This file provides APIs for dynamically configuring USB interfaces at runtime
 * for both host and device stacks. It supports adding/removing interfaces,
 * configuring endpoints, and registering handlers for different USB classes.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hw/hurricane_hw_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of endpoints per interface
 */
#define MAX_ENDPOINTS_PER_INTERFACE 16

/**
 * @brief Maximum number of string descriptors
 */
#define MAX_STRING_DESCRIPTORS 10

/**
 * @brief USB interface handler type
 */
typedef enum {
    INTERFACE_HANDLER_NONE = 0,    /**< No handler assigned */
    INTERFACE_HANDLER_HID,         /**< HID class handler */
    INTERFACE_HANDLER_CDC,         /**< CDC class handler */
    INTERFACE_HANDLER_MSC,         /**< Mass Storage class handler */
    INTERFACE_HANDLER_CUSTOM       /**< Custom handler */
} hurricane_interface_handler_type_t;

/**
 * @brief USB event types
 */
typedef enum {
    USB_EVENT_DEVICE_ATTACHED,     /**< Device attached to host */
    USB_EVENT_DEVICE_DETACHED,     /**< Device detached from host */
    USB_EVENT_HOST_CONNECTED,      /**< Host connected to device */
    USB_EVENT_HOST_DISCONNECTED,   /**< Host disconnected from device */
    USB_EVENT_INTERFACE_ENABLED,   /**< Interface enabled */
    USB_EVENT_INTERFACE_DISABLED,  /**< Interface disabled */
    USB_EVENT_ENDPOINT_DATA,       /**< Data received on endpoint */
    USB_EVENT_CONTROL_REQUEST      /**< Control request received */
} hurricane_usb_event_t;

/**
 * @brief USB endpoint descriptor
 */
typedef struct {
    uint8_t ep_address;            /**< Endpoint address including direction bit */
    uint8_t ep_attributes;         /**< Endpoint attributes (type, etc.) */
    uint16_t ep_max_packet_size;   /**< Maximum packet size */
    uint8_t ep_interval;           /**< Polling interval */
    bool configured;               /**< Whether endpoint is configured */
} hurricane_endpoint_descriptor_t;

/**
 * @brief USB interface descriptor
 */
typedef struct {
    uint8_t interface_num;         /**< Interface number */
    uint8_t interface_class;       /**< USB class code */
    uint8_t interface_subclass;    /**< USB subclass code */
    uint8_t interface_protocol;    /**< Protocol code */
    uint8_t num_endpoints;         /**< Number of endpoints */
    hurricane_interface_handler_type_t handler_type;  /**< Handler type */
    void* handler_data;            /**< Handler-specific data */
    bool (*control_handler)(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);  /**< Control request handler */
} hurricane_interface_descriptor_t;

/**
 * @brief Interface registry entry
 */
typedef struct interface_registry_entry {
    hurricane_interface_descriptor_t descriptor;   /**< Interface descriptor */
    hurricane_endpoint_descriptor_t endpoints[MAX_ENDPOINTS_PER_INTERFACE];  /**< Endpoints */
    bool active;                   /**< Whether interface is active */
    struct interface_registry_entry* next;  /**< Next interface in list */
} hurricane_interface_registry_entry_t;

/**
 * @brief Host class handler functions
 */
typedef struct {
    bool (*match_callback)(uint8_t device_class, uint8_t device_subclass, uint8_t device_protocol);  /**< Match function */
    void (*attach_callback)(void* device);  /**< Device attached callback */
    void (*detach_callback)(void* device);  /**< Device detached callback */
    bool (*control_callback)(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length);  /**< Control transfer callback */
    void (*data_callback)(uint8_t endpoint, void* buffer, uint16_t length);  /**< Data transfer callback */
} hurricane_host_class_handler_t;

/**
 * @brief Device descriptors structure
 */
typedef struct {
    uint8_t* device_descriptor;           /**< Device descriptor */
    uint16_t device_descriptor_length;     /**< Device descriptor length */
    uint8_t* config_descriptor;           /**< Configuration descriptor */
    uint16_t config_descriptor_length;     /**< Configuration descriptor length */
    uint8_t* string_descriptors[MAX_STRING_DESCRIPTORS];  /**< String descriptors */
    uint16_t string_descriptor_lengths[MAX_STRING_DESCRIPTORS];  /**< String descriptor lengths */
    uint8_t* hid_report_descriptor;       /**< HID report descriptor */
    uint16_t hid_report_descriptor_length; /**< HID report descriptor length */
} hurricane_device_descriptors_t;

/**
 * @brief Initialize the interface manager
 *
 * This function initializes the interface manager for both host and device modes.
 * It should be called before any other interface manager functions.
 */
void hurricane_interface_manager_init(void);

/**
 * @brief Deinitialize the interface manager
 *
 * This function cleans up all resources allocated by the interface manager.
 * It should be called during system shutdown to prevent memory leaks.
 */
void hurricane_interface_manager_deinit(void);

/**
 * @brief Add a device-mode interface at runtime
 *
 * @param interface_num Interface number
 * @param interface_class USB class code
 * @param interface_subclass USB subclass code
 * @param interface_protocol Protocol code
 * @param descriptor Pointer to interface descriptor
 * @return 0 on success, negative error code on failure
 */
int hurricane_add_device_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol,
    const hurricane_interface_descriptor_t* descriptor
);

/**
 * @brief Remove a device-mode interface at runtime
 *
 * @param interface_num Interface number to remove
 * @return 0 on success, negative error code on failure
 */
int hurricane_remove_device_interface(uint8_t interface_num);

/**
 * @brief Configure a device-mode endpoint at runtime
 *
 * @param interface_num Interface number
 * @param ep_address Endpoint address including direction bit
 * @param ep_attributes Endpoint attributes
 * @param ep_max_packet_size Maximum packet size
 * @param ep_interval Polling interval
 * @return 0 on success, negative error code on failure 
 */
int hurricane_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval
);

/**
 * @brief Register a handler for device interface control requests
 *
 * @param interface_num Interface number
 * @param handler Handler function
 */
void hurricane_device_interface_register_control_handler(
    uint8_t interface_num,
    bool (*handler)(hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t* length)
);

/**
 * @brief Register handler for host-mode device class at runtime
 *
 * @param device_class USB class code
 * @param device_subclass USB subclass code
 * @param device_protocol Protocol code
 * @param handler Pointer to handler structure
 * @return 0 on success, negative error code on failure
 */
int hurricane_register_host_class_handler(
    uint8_t device_class,
    uint8_t device_subclass,
    uint8_t device_protocol,
    const hurricane_host_class_handler_t* handler
);

/**
 * @brief Unregister handler for host-mode device class
 *
 * @param device_class USB class code
 * @param device_subclass USB subclass code
 * @param device_protocol Protocol code
 * @return 0 on success, negative error code on failure
 */
int hurricane_unregister_host_class_handler(
    uint8_t device_class,
    uint8_t device_subclass,
    uint8_t device_protocol
);

/**
 * @brief Notify events to registered handlers
 *
 * @param event Event type
 * @param interface_num Interface number
 * @param event_data Event-specific data
 */
void hurricane_interface_notify_event(
    hurricane_usb_event_t event,
    uint8_t interface_num,
    void* event_data
);

/**
 * @brief Extended notify events function with response callback for control requests
 *
 * This function extends hurricane_interface_notify_event to include a response callback
 * for control requests. This allows the device HAL to wait for responses to control
 * requests.
 *
 * @param event Event type
 * @param interface_num Interface number
 * @param event_data Event data
 * @param response_cb Callback for control request responses (can be NULL for no callback)
 * @return true if handled synchronously, false if response will be async or not handled
 */
bool hurricane_interface_notify_event_with_response(
    hurricane_usb_event_t event,
    uint8_t interface_num,
    void* event_data,
    void (*response_cb)(uint8_t interface_num, bool handled, void* buffer, uint16_t length)
);

/**
 * @brief Update device descriptors at runtime
 * 
 * @param descriptors Pointer to device descriptors structure
 * @return 0 on success, negative error code on failure
 */
int hurricane_device_update_descriptors(hurricane_device_descriptors_t* descriptors);

/**
 * @brief Update device HID report descriptor at runtime
 * 
 * @param report_descriptor Pointer to HID report descriptor
 * @param length Length of the descriptor
 * @return 0 on success, negative error code on failure
 */
int hurricane_device_update_report_descriptor(uint8_t* report_descriptor, uint16_t length);

/**
 * @brief Trigger USB device reset/reconnect
 * 
 * This function triggers a disconnect/connect cycle to notify the host
 * of device changes.
 * 
 * @return 0 on success, negative error code on failure
 */
int hurricane_device_trigger_reset(void);

/**
 * @brief Get interface descriptor for a given interface number
 * 
 * @param interface_num Interface number
 * @return Pointer to interface descriptor or NULL if not found
 */
const hurricane_interface_descriptor_t* hurricane_get_device_interface(uint8_t interface_num);

/**
 * @brief Get endpoint descriptor for a given endpoint address
 * 
 * @param interface_num Interface number
 * @param ep_address Endpoint address
 * @return Pointer to endpoint descriptor or NULL if not found
 */
const hurricane_endpoint_descriptor_t* hurricane_get_device_endpoint(
    uint8_t interface_num, 
    uint8_t ep_address
);

#ifdef __cplusplus
}
#endif