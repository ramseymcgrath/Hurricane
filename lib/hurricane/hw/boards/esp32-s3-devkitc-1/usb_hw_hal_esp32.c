#include "hw/usb_hw_hal.h"
#if defined(PLATFORM_ESP32)
#include "tusb.h"
#include "esp_log.h"

static const char* TAG = "usb_hw_esp32";

// Track device connection
static bool device_connected = false;

void usb_hw_init(void)
{
    ESP_LOGI(TAG, "Initializing TinyUSB host stack...");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false
    };
    tinyusb_driver_install(&tusb_cfg);

    ESP_LOGI(TAG, "TinyUSB host initialized");
}

void usb_hw_task(void)
{
    tuh_task();
}

int usb_hw_device_connected(void)
{
    return device_connected ? 1 : 0;
}

int usb_hw_send_control_setup(const usb_hw_setup_packet_t* setup)
{
    ESP_LOGI(TAG, "Sending control setup");

    // Fill TinyUSB control request structure
    tusb_control_request_t req = {
        .bmRequestType = setup->bmRequestType,
        .bRequest = setup->bRequest,
        .wValue = setup->wValue,
        .wIndex = setup->wIndex,
        .wLength = setup->wLength,
    };

    // Assume device 1 for now (you can generalize later)
    if (!tuh_control_xfer(1, &req, NULL, NULL)) {
        ESP_LOGE(TAG, "Control transfer failed");
        return -1;
    }
    return 0;
}

int usb_hw_send_control_data(const uint8_t* buffer, uint16_t length)
{
    ESP_LOGW(TAG, "Sending control data not yet implemented");
    return 0;
}

int usb_hw_receive_control_data(uint8_t* buffer, uint16_t length)
{
    ESP_LOGW(TAG, "Receiving control data not yet implemented");
    return 0;
}

void usb_hw_reset_bus(void)
{
    ESP_LOGW(TAG, "Bus reset not implemented yet on ESP32");
}
#endif