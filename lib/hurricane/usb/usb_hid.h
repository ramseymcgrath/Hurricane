#pragma once

#include "core/hurricane_usb.h"
#include "core/usb_descriptor.h"
#include "usb/usb_control.h"
#include "hw/hurricane_hw_hal.h"

// Initialize the HID device
void hurricane_hid_init(hurricane_device_t* dev);

// Process HID reports
void hurricane_hid_task(hurricane_device_t* dev);

// Handle HID class-specific requests
int hurricane_hid_class_request(hurricane_device_t* dev, hurricane_usb_setup_packet_t* setup);

// Fetch report descriptor during enumeration
int hurricane_hid_fetch_report_descriptor(hurricane_device_t* dev);