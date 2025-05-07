/*
 * @file usb_interface_manager.c
 * @brief Implementation of the dynamic USB interface configuration and management
 *
 * pthread has been removed. Thread‑safety is now optional at compile time.
 * Define HURRICANE_USE_THREADING if you actually have an RTOS or POSIX layer
 * that provides pthreads (or if you map these macros to your own mutex API).
 */

#include "usb_interface_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hw/hurricane_hw_hal.h"

/* -------------------------------------------------------------------------- */
/*                          Optional mutex abstraction                        */
/* -------------------------------------------------------------------------- */
#ifdef HURRICANE_USE_THREADING
#include <pthread.h>
static pthread_mutex_t interface_manager_mutex = PTHREAD_MUTEX_INITIALIZER;
#define INTERFACE_MANAGER_LOCK()   pthread_mutex_lock(&interface_manager_mutex)
#define INTERFACE_MANAGER_UNLOCK() pthread_mutex_unlock(&interface_manager_mutex)
#else
#define INTERFACE_MANAGER_LOCK()   ((void)0)
#define INTERFACE_MANAGER_UNLOCK() ((void)0)
#endif

/* -------------------------------------------------------------------------- */
/*                              Private definitions                           */
/* -------------------------------------------------------------------------- */
#define MAX_HOST_CLASS_HANDLERS      8
#define MAX_INTERFACE_REGISTRY_ENTRIES 16

/* Error codes */
#define HURRICANE_ERROR_NONE            0
#define HURRICANE_ERROR_INVALID_PARAM  -1
#define HURRICANE_ERROR_NO_MEMORY      -2
#define HURRICANE_ERROR_NOT_FOUND      -3
#define HURRICANE_ERROR_ALREADY_EXISTS -4

/* Registry for device‑mode interfaces */
static hurricane_interface_registry_entry_t *device_interface_registry = NULL;

/* Registry for host‑mode class handlers */
typedef struct {
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    hurricane_host_class_handler_t handler;
    bool active;
} host_class_handler_entry_t;

static host_class_handler_entry_t host_class_handlers[MAX_HOST_CLASS_HANDLERS];
static uint8_t num_host_class_handlers = 0;

/* Current device descriptors */
static hurricane_device_descriptors_t current_device_descriptors = {0};

/* Forward declarations of helper functions */
static hurricane_interface_registry_entry_t *find_device_interface(uint8_t interface_num);
static hurricane_endpoint_descriptor_t *find_device_endpoint(hurricane_interface_registry_entry_t *interface, uint8_t ep_address);
static host_class_handler_entry_t *find_host_class_handler(uint8_t device_class, uint8_t device_subclass, uint8_t device_protocol);

/* -------------------------------------------------------------------------- */
/*                                API implementation                          */
/* -------------------------------------------------------------------------- */
void hurricane_interface_manager_init(void)
{
    INTERFACE_MANAGER_LOCK();

    /* Initialise device interface registry */
    device_interface_registry = NULL;

    /* Initialise host class handlers */
    memset(host_class_handlers, 0, sizeof(host_class_handlers));
    num_host_class_handlers = 0;

    /* Clear current descriptors */
    memset(&current_device_descriptors, 0, sizeof(current_device_descriptors));

    INTERFACE_MANAGER_UNLOCK();
    printf("[Interface Manager] Initialised (threading %s)\n",
           #ifdef HURRICANE_USE_THREADING
           "enabled"
           #else
           "disabled"
           #endif
           );
}

/**
 * @brief De‑initialise the interface manager and free all resources.
 */
void hurricane_interface_manager_deinit(void)
{
    INTERFACE_MANAGER_LOCK();

    hurricane_interface_registry_entry_t *current = device_interface_registry;
    while (current) {
        hurricane_interface_registry_entry_t *next = current->next;
        free(current);
        current = next;
    }
    device_interface_registry = NULL;

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

    INTERFACE_MANAGER_UNLOCK();
    printf("[Interface Manager] De‑initialised and resources freed\n");
}

/* --------------------------- Device‑side helpers -------------------------- */
int hurricane_add_device_interface(uint8_t interface_num,
                                   uint8_t interface_class,
                                   uint8_t interface_subclass,
                                   uint8_t interface_protocol,
                                   const hurricane_interface_descriptor_t *descriptor)
{
    INTERFACE_MANAGER_LOCK();
    if (!descriptor) {
        printf("[Interface Manager] Error: Null descriptor\n");
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_INVALID_PARAM;
    }

    if (find_device_interface(interface_num)) {
        printf("[Interface Manager] Error: Interface %d already exists\n", interface_num);
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_ALREADY_EXISTS;
    }

    hurricane_interface_registry_entry_t *new_entry = malloc(sizeof(*new_entry));
    if (!new_entry) {
        printf("[Interface Manager] Error: Out of memory for interface %d\n", interface_num);
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_NO_MEMORY;
    }

    memcpy(&new_entry->descriptor, descriptor, sizeof(hurricane_interface_descriptor_t));
    memset(new_entry->endpoints, 0, sizeof(new_entry->endpoints));

    new_entry->active = true;
    new_entry->next = device_interface_registry;
    device_interface_registry = new_entry;

    int hw = hurricane_hw_device_configure_interface(interface_num, interface_class,
                                                     interface_subclass, interface_protocol);
    if (hw) {
        printf("[Interface Manager] Warning: HW interface cfg returned %d\n", hw);
    }

    printf("[Interface Manager] Added interface %d (class %d/%d/%d)\n",
           interface_num, interface_class, interface_subclass, interface_protocol);
    hurricane_interface_notify_event(USB_EVENT_INTERFACE_ENABLED, interface_num, NULL);
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

int hurricane_remove_device_interface(uint8_t interface_num)
{
    INTERFACE_MANAGER_LOCK();
    hurricane_interface_registry_entry_t *cur = device_interface_registry, *prev = NULL;
    while (cur) {
        if (cur->descriptor.interface_num == interface_num) {
            if (prev) prev->next = cur->next; else device_interface_registry = cur->next;
            hurricane_interface_notify_event(USB_EVENT_INTERFACE_DISABLED, interface_num, NULL);
            free(cur);
            printf("[Interface Manager] Removed interface %d\n", interface_num);
            INTERFACE_MANAGER_UNLOCK();
            return HURRICANE_ERROR_NONE;
        }
        prev = cur;
        cur = cur->next;
    }
    printf("[Interface Manager] Error: Interface %d not found\n", interface_num);
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NOT_FOUND;
}

int hurricane_device_configure_endpoint(uint8_t interface_num, uint8_t ep_address,
                                        uint8_t ep_attributes, uint16_t ep_max_packet_size,
                                        uint8_t ep_interval)
{
    INTERFACE_MANAGER_LOCK();
    hurricane_interface_registry_entry_t *iface = find_device_interface(interface_num);
    if (!iface) {
        printf("[Interface Manager] Error: Interface %d not found\n", interface_num);
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_NOT_FOUND;
    }

    hurricane_endpoint_descriptor_t *ep = find_device_endpoint(iface, ep_address);
    if (!ep) {
        for (int i = 0; i < MAX_ENDPOINTS_PER_INTERFACE; i++) {
            if (!iface->endpoints[i].configured) {
                ep = &iface->endpoints[i];
                break;
            }
        }
        if (!ep) {
            printf("[Interface Manager] Error: No EP slots for iface %d\n", interface_num);
            INTERFACE_MANAGER_UNLOCK();
            return HURRICANE_ERROR_NO_MEMORY;
        }
    }
    ep->ep_address = ep_address;
    ep->ep_attributes = ep_attributes;
    ep->ep_max_packet_size = ep_max_packet_size;
    ep->ep_interval = ep_interval;
    ep->configured = true;

    int hw = hurricane_hw_device_configure_endpoint(interface_num, ep_address, ep_attributes,
                                                    ep_max_packet_size, ep_interval);
    if (hw) {
        printf("[Interface Manager] Warning: HW EP cfg returned %d\n", hw);
    }

    printf("[Interface Manager] Configured EP 0x%02X on iface %d\n", ep_address, interface_num);
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

void hurricane_device_interface_register_control_handler(uint8_t interface_num,
        bool (*handler)(hurricane_usb_setup_packet_t *, void *, uint16_t *))
{
    INTERFACE_MANAGER_LOCK();
    hurricane_interface_registry_entry_t *iface = find_device_interface(interface_num);
    if (!iface) {
        printf("[Interface Manager] Error: Interface %d not found for ctl handler\n", interface_num);
        INTERFACE_MANAGER_UNLOCK();
        return;
    }
    iface->descriptor.control_handler = handler;
    printf("[Interface Manager] Registered control handler for iface %d\n", interface_num);
    INTERFACE_MANAGER_UNLOCK();
}

/* --------------------------- Host‑side helpers --------------------------- */
int hurricane_register_host_class_handler(uint8_t device_class, uint8_t device_subclass,
                                          uint8_t device_protocol,
                                          const hurricane_host_class_handler_t *handler)
{
    INTERFACE_MANAGER_LOCK();
    if (!handler) {
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_INVALID_PARAM;
    }
    if (find_host_class_handler(device_class, device_subclass, device_protocol)) {
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_ALREADY_EXISTS;
    }
    if (num_host_class_handlers >= MAX_HOST_CLASS_HANDLERS) {
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_NO_MEMORY;
    }
    host_class_handler_entry_t *slot = &host_class_handlers[num_host_class_handlers++];
    slot->device_class = device_class;
    slot->device_subclass = device_subclass;
    slot->device_protocol = device_protocol;
    memcpy(&slot->handler, handler, sizeof(hurricane_host_class_handler_t));
    slot->active = true;
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

int hurricane_unregister_host_class_handler(uint8_t device_class, uint8_t device_subclass,
                                            uint8_t device_protocol)
{
    INTERFACE_MANAGER_LOCK();
    host_class_handler_entry_t *h = find_host_class_handler(device_class, device_subclass, device_protocol);
    if (!h) {
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_NOT_FOUND;
    }
    h->active = false;
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

/* ---------------------------- Event dispatch ----------------------------- */
void hurricane_interface_notify_event(hurricane_usb_event_t event, uint8_t interface_num, void *event_data)
{
    hurricane_interface_notify_event_with_response(event, interface_num, event_data, NULL);
}

bool hurricane_interface_notify_event_with_response(hurricane_usb_event_t event, uint8_t interface_num,
                                                    void *event_data,
                                                    void (*rsp)(uint8_t, bool, void *, uint16_t))
{
    INTERFACE_MANAGER_LOCK();
    /* Device‑side control requests */
    if (event == USB_EVENT_CONTROL_REQUEST && event_data) {
        hurricane_interface_registry_entry_t *iface = find_device_interface(interface_num);
        if (iface && iface->descriptor.control_handler) {
            hurricane_usb_setup_packet_t *setup = event_data;
            uint16_t len = setup->wLength;
            uint8_t *buf = NULL;
            if ((setup->bmRequestType & 0x80) && len) {
                buf = malloc(len);
                if (!buf) { INTERFACE_MANAGER_UNLOCK(); return false; }
            }
            bool handled = iface->descriptor.control_handler(setup, buf, &len);
            if (rsp && handled) rsp(interface_num, handled, buf, len);
            if (buf) free(buf);
            INTERFACE_MANAGER_UNLOCK();
            return handled;
        }
    }

    /* Host‑side attach/detach (placeholder HID example) */
    if ((event == USB_EVENT_DEVICE_ATTACHED || event == USB_EVENT_DEVICE_DETACHED) && event_data) {
        uint8_t cls = 3, sub = 0, proto = 0; /* TODO: real parse */
        host_class_handler_entry_t *h = find_host_class_handler(cls, sub, proto);
        if (h && ((event == USB_EVENT_DEVICE_ATTACHED && h->handler.attach_callback) ||
                  (event == USB_EVENT_DEVICE_DETACHED && h->handler.detach_callback))) {
            if (event == USB_EVENT_DEVICE_ATTACHED) h->handler.attach_callback(event_data);
            else h->handler.detach_callback(event_data);
        }
    }

    INTERFACE_MANAGER_UNLOCK();
    return false;
}

/* -------------------------- Descriptor updates --------------------------- */
int hurricane_device_update_descriptors(hurricane_device_descriptors_t *desc)
{
    INTERFACE_MANAGER_LOCK();
    if (!desc) { INTERFACE_MANAGER_UNLOCK(); return HURRICANE_ERROR_INVALID_PARAM; }
    memcpy(&current_device_descriptors, desc, sizeof(*desc));

    if (desc->device_descriptor && desc->config_descriptor) {
        hurricane_hw_device_set_descriptors(desc->device_descriptor, desc->device_descriptor_length,
                                            desc->config_descriptor, desc->config_descriptor_length);
    }
    if (desc->hid_report_descriptor) {
        hurricane_hw_device_set_hid_report_descriptor(desc->hid_report_descriptor,
                                                      desc->hid_report_descriptor_length);
    }
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

int hurricane_device_update_report_descriptor(uint8_t *report_desc, uint16_t len)
{
    INTERFACE_MANAGER_LOCK();
    if (!report_desc || !len) { INTERFACE_MANAGER_UNLOCK(); return HURRICANE_ERROR_INVALID_PARAM; }
    if (current_device_descriptors.hid_report_descriptor) {
        free(current_device_descriptors.hid_report_descriptor);
    }
    current_device_descriptors.hid_report_descriptor = malloc(len);
    if (!current_device_descriptors.hid_report_descriptor) {
        INTERFACE_MANAGER_UNLOCK();
        return HURRICANE_ERROR_NO_MEMORY;
    }
    memcpy(current_device_descriptors.hid_report_descriptor, report_desc, len);
    current_device_descriptors.hid_report_descriptor_length = len;
    hurricane_hw_device_set_hid_report_descriptor(report_desc, len);
    INTERFACE_MANAGER_UNLOCK();
    return HURRICANE_ERROR_NONE;
}

int hurricane_device_trigger_reset(void)
{
    printf("[Interface Manager] Triggering USB device reset\n");
    hurricane_hw_device_reset();
    return HURRICANE_ERROR_NONE;
}

const hurricane_interface_descriptor_t *hurricane_get_device_interface(uint8_t interface_num)
{
    INTERFACE_MANAGER_LOCK();
    hurricane_interface_registry_entry_t *iface = find_device_interface(interface_num);
    const hurricane_interface_descriptor_t *ret = iface ? &iface->descriptor : NULL;
    INTERFACE_MANAGER_UNLOCK();
    return ret;
}

const hurricane_endpoint_descriptor_t *hurricane_get_device_endpoint(uint8_t interface_num, uint8_t ep_address)
{
    INTERFACE_MANAGER_LOCK();
    hurricane_interface_registry_entry_t *iface = find_device_interface(interface_num);
    const hurricane_endpoint_descriptor_t *ret = iface ? find_device_endpoint(iface, ep_address) : NULL;
    INTERFACE_MANAGER_UNLOCK();
    return ret;
}

/* -------------------------------------------------------------------------- */
/*                              Internal helpers                              */
/* -------------------------------------------------------------------------- */
static hurricane_interface_registry_entry_t *find_device_interface(uint8_t interface_num)
{
    for (hurricane_interface_registry_entry_t *cur = device_interface_registry; cur; cur = cur->next) {
        if (cur->active && cur->descriptor.interface_num == interface_num) return cur;
    }
    return NULL;
}

static hurricane_endpoint_descriptor_t *find_device_endpoint(hurricane_interface_registry_entry_t *iface, uint8_t ep_address)
{
    if (!iface) return NULL;
    for (int i = 0; i < MAX_ENDPOINTS_PER_INTERFACE; i++) {
        if (iface->endpoints[i].configured && iface->endpoints[i].ep_address == ep_address) return &iface->endpoints[i];
    }
    return NULL;
}

static host_class_handler_entry_t *find_host_class_handler(uint8_t cls, uint8_t sub, uint8_t proto)
{
    /* Pass 1: exact */
    for (int i = 0; i < num_host_class_handlers; i++) {
        if (host_class_handlers[i].active && host_class_handlers[i].device_class == cls &&
            host_class_handlers[i].device_subclass == sub && host_class_handlers[i].device_protocol == proto)
            return &host_class_handlers[i];
    }
    /* Pass 2: class + wildcards */
    for (int i = 0; i < num_host_class_handlers; i++) {
        if (host_class_handlers[i].active && host_class_handlers[i].device_class == cls &&
            (!host_class_handlers[i].device_subclass || host_class_handlers[i].device_subclass == sub) &&
            (!host_class_handlers[i].device_protocol || host_class_handlers[i].device_protocol == proto))
            return &host_class_handlers[i];
    }
    return NULL;
}
