#ifndef USB_HW_HAL_H
#define USB_HW_HAL_H

#include <stdint.h>
#include "usb_control.h"

// Must be implemented per board:

// Bus management
void usb_hw_reset_bus(void);
void usb_hw_enable_host_mode(void);
void usb_hw_disable_host_mode(void);

// Control transfer management
int usb_hw_send_setup(const usb_setup_packet_t* setup);
int usb_hw_receive_control_data(uint8_t* buffer, uint16_t length);
int usb_hw_send_control_data(const uint8_t* buffer, uint16_t length);

// Transaction helpers
int usb_hw_control_in(uint8_t endpoint, uint8_t* buffer, uint16_t length);
int usb_hw_control_out(uint8_t endpoint, const uint8_t* buffer, uint16_t length);

// Optional (future): interrupt handlers, connection detect, suspend/resume

#endif // USB_HW_HAL_H
