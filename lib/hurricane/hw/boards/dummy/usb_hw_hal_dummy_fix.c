#include "hw/hurricane_hw_hal.h"
#include <stdio.h>

/**
 * @brief Configure a USB endpoint in device mode (dummy implementation)
 */
int hurricane_hw_device_configure_endpoint(
    uint8_t interface_num,
    uint8_t ep_address,
    uint8_t ep_attributes,
    uint16_t ep_max_packet_size,
    uint8_t ep_interval
) {
    printf("[stub-hal-fix] hurricane_hw_device_configure_endpoint(): interface=%d, ep=%02x\n", 
           interface_num, ep_address);
    return 0; // Return success
}

/**
 * @brief Configure a USB interface in device mode (dummy implementation)
 */
int hurricane_hw_device_configure_interface(
    uint8_t interface_num,
    uint8_t interface_class,
    uint8_t interface_subclass,
    uint8_t interface_protocol
) {
    printf("[stub-hal-fix] hurricane_hw_device_configure_interface(): interface=%d, class=%02x\n", 
           interface_num, interface_class);
    return 0; // Return success
}

/**
 * @brief Reset the USB device controller (dummy implementation)
 */
void hurricane_hw_device_reset(void) {
    printf("[stub-hal-fix] hurricane_hw_device_reset()\n");
}

/**
 * @brief Set device descriptors (dummy implementation)
 */
int hurricane_hw_device_set_descriptors(
    const uint8_t* device_desc,
    uint16_t device_desc_length,
    const uint8_t* config_desc,
    uint16_t config_desc_length
) {
    printf("[stub-hal-fix] hurricane_hw_device_set_descriptors(): device_len=%d, config_len=%d\n", 
           device_desc_length, config_desc_length);
    return 0; // Return success
}

/**
 * @brief Set HID report descriptor (dummy implementation)
 */
int hurricane_hw_device_set_hid_report_descriptor(
    const uint8_t* report_desc,
    uint16_t report_desc_length
) {
    printf("[stub-hal-fix] hurricane_hw_device_set_hid_report_descriptor(): len=%d\n", 
           report_desc_length);
    return 0; // Return success
}