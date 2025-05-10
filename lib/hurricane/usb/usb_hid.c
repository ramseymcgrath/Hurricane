#include "usb_hid.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// HID mouse report format (based on standard boot protocol)
typedef struct {
    uint8_t buttons;  // Bit 0: Left, Bit 1: Right, Bit 2: Middle
    int8_t x;         // Relative X movement
    int8_t y;         // Relative Y movement
    int8_t wheel;     // Vertical wheel
} mouse_report_t;

void hurricane_hid_init(hurricane_device_t* dev) {
    dev->hid_device->report_id = 0;
    dev->hid_device->protocol = 1; // Default to boot protocol
    dev->hid_device->idle_rate = 0;
    dev->hid_device->report_descriptor_length = 0;

    // Set the HID idle rate
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE,
        .bRequest = 0x0A, // SET_IDLE
        .wValue = (dev->hid_device->idle_rate << 8),
        .wIndex = dev->hid_device->interface_number, // should track correct iface
        .wLength = 0,
    };
    hurricane_hw_control_transfer(&setup, NULL, 0);

    // TODO: Optionally set protocol to boot/report
}

static void parse_mouse_report(uint8_t* buffer, int length) {
    if (length < 3) {
        // Not enough data for a mouse report
        return;
    }
    
    mouse_report_t report;
    memset(&report, 0, sizeof(report));
    
    // Basic boot protocol mouse has 3 bytes minimum
    report.buttons = buffer[0];
    report.x = buffer[1];
    report.y = buffer[2];
    
    // Optional wheel data if available
    if (length > 3) {
        report.wheel = buffer[3];
    }
    
    // Print a human-readable description of the mouse state
    printf("[MOUSE] Buttons: %s%s%s | X: %d, Y: %d, Wheel: %d\n",
           (report.buttons & 0x01) ? "LEFT " : "",
           (report.buttons & 0x02) ? "RIGHT " : "",
           (report.buttons & 0x04) ? "MIDDLE " : "",
           report.x, report.y, report.wheel);
}

void hurricane_hid_task(hurricane_device_t* dev) {
    uint8_t buffer[64];
    int res = hurricane_hw_interrupt_in_transfer(dev->addr, buffer, sizeof(buffer));
    if (res > 0) {
        printf("[HID] Received %d bytes:\n", res);
        for (int i = 0; i < res; i++) {
            printf(" 0x%02X", buffer[i]);
        }
        printf("\n");
        
        // Attempt to parse it as a mouse report
        parse_mouse_report(buffer, res);
    }
}

int hurricane_hid_class_request(hurricane_device_t* dev, hurricane_usb_setup_packet_t* setup) {
    if (setup->bRequest == USB_REQ_GET_DESCRIPTOR &&
        (setup->wValue >> 8) == USB_DESC_TYPE_REPORT) {
        
        printf("[HID] Host requested HID report descriptor\n");

        if (dev->hid_device->report_descriptor_length > 0) {
            hurricane_hw_control_transfer(
                setup,
                dev->hid_device->report_descriptor,
                dev->hid_device->report_descriptor_length
            );
            return 0;
        } else {
            printf("[HID] Error: No report descriptor cached!\n");
            return -1;
        }
    }
    return -1; // Unsupported request
}

// New helper during enumeration
int hurricane_hid_fetch_report_descriptor(hurricane_device_t* dev) {
    hurricane_usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_INTERFACE | 0x80, // IN transfer
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (USB_DESC_TYPE_REPORT << 8),
        .wIndex = dev->hid_device->interface_number,
        .wLength = sizeof(dev->hid_device->report_descriptor),
    };

    int ret = hurricane_hw_control_transfer(&setup, dev->hid_device->report_descriptor, sizeof(dev->hid_device->report_descriptor));
    if (ret >= 0) {
        dev->hid_device->report_descriptor_length = ret;
        printf("[HID] Fetched %d bytes of HID report descriptor\n", ret);
        return 0;
    } else {
        printf("[HID] Failed to fetch HID report descriptor\n");
        return -1;
    }
}

// Callback function pointers
static void (*hid_send_callback)(uint8_t* buffer, uint16_t length) = NULL;
static void (*hid_receive_callback)(uint8_t* buffer, uint16_t length) = NULL;

/**
 * @brief Register HID device callbacks for sending and receiving reports
 *
 * @param send_callback Function called after a report is sent to host
 * @param receive_callback Function called after a report is received from host
 */
void hurricane_device_hid_register_callbacks(
    void (*send_callback)(uint8_t* buffer, uint16_t length),
    void (*receive_callback)(uint8_t* buffer, uint16_t length)
) {
    hid_send_callback = send_callback;
    hid_receive_callback = receive_callback;
}

/**
 * @brief Send HID report to host
 *
 * @param buffer Pointer to report data
 * @param length Size of report in bytes
 * @return Number of bytes sent, or negative error code
 */
int hurricane_device_hid_send_report(uint8_t* buffer, uint16_t length) {
    // Use IN endpoint 1 for HID reports (standard for HID devices)
    const uint8_t hid_endpoint = 0x81; // 0x80 is IN direction, 0x01 is endpoint number
    
    // Send the report via interrupt IN transfer
    int result = hurricane_hw_device_interrupt_in_transfer(hid_endpoint, buffer, length);
    
    // Call the send callback if registered and transfer was successful
    if (result > 0 && hid_send_callback != NULL) {
        hid_send_callback(buffer, result);
    }
    
    return result;
}
