#pragma once

// Hurricane USB - Master Header
#include "core/hurricane_usb.h"
#include "core/usb_host_controller.h"
#include "usb/usb_control.h"
#include "core/usb_descriptor.h"
#include "hw/hurricane_hw_hal.h"


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
