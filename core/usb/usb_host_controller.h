#ifndef USB_HOST_CONTROLLER_H
#define USB_HOST_CONTROLLER_H

#include <stdint.h>

typedef enum {
    DEVICE_STATE_DEFAULT,
    DEVICE_STATE_ADDRESS,
    DEVICE_STATE_CONFIGURED,
    DEVICE_STATE_ERROR
} usb_device_state_t;

typedef struct {
    usb_device_state_t state;
    uint8_t device_address;
    //TODO: Add other device-specific information etc etc
} usb_device_t;

void usb_host_init(void);
void usb_host_poll(void);

#endif // USB_HOST_CONTROLLER_H
