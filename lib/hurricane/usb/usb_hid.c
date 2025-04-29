#include "usb/usb_hid.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

void hurricane_hid_task(hurricane_device_t* dev) {
    uint8_t buffer[64];
    int res = hurricane_hw_interrupt_in_transfer(dev->addr, buffer, sizeof(buffer));
    if (res > 0) {
        printf("[HID] Received %d bytes:\n", res);
        for (int i = 0; i < res; i++) {
            printf(" 0x%02X", buffer[i]);
        }
        printf("\n");
        // TODO: Parse known HID report types here
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
