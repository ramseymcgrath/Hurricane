#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t max_power_ma;
    uint8_t version_major;
    uint8_t version_minor;
    const char* manufacturer;
    const char* product;
} device_config_t;

const device_config_t* get_device_config(void);

#define MAX_USB_DEVICES 10
#define MAX_USB_ENDPOINTS 16
#define MAX_USB_INTERFACES 8
#define MAX_USB_STRING_DESCRIPTORS 10
#define MAX_USB_DESCRIPTOR_SIZE 512


#endif // DEVICE_CONFIG_H