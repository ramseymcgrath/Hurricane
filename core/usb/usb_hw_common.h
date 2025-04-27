#ifndef USB_HW_COMMON_H
#define USB_HW_COMMON_H

#include "usb_control.h"
#include <stdint.h>

// Reset the USB bus (simulate or trigger real bus reset)
void usb_hw_reset_bus(void);

// Send a USB setup packet
void usb_hw_send_setup_packet(const usb_setup_packet_t* setup);

// Receive data from the device (for control read transfers)
int usb_hw_receive_data(uint8_t* buffer, uint16_t length);

#endif // USB_HW_COMMON_H
