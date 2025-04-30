#include "hw/hurricane_hw_hal.h"
#include "core/usb_descriptor.h" // For USB_DESC_TYPE_DEVICE 
#include "usb/usb_control.h"     // For USB_REQ_GET_DESCRIPTOR
#include <stdio.h>
#include <string.h>              // For memcpy

// Global variables used by tests to verify correct behavior
hurricane_usb_setup_packet_t last_setup_sent;
uint8_t last_control_data_sent[64];
size_t last_control_data_length = 0;

// These variables will be accessed by the test file
uint8_t test_address_set = 0;
uint8_t test_descriptor_requested = 0;

void hurricane_hw_init(void) {
    printf("[stub-hal] hurricane_hw_init()\n");
}

void hurricane_hw_poll(void) {
    // no-op
}

int hurricane_hw_device_connected(void) {
    return 1; // Pretend a device is always connected
}

int hurricane_hw_control_transfer(const hurricane_usb_setup_packet_t* setup, void* buffer, uint16_t length) {
    printf("[stub-hal] hurricane_hw_control_transfer(): Request=0x%02X\n", setup->bRequest);
    
    // Save the setup packet for tests to verify
    memcpy(&last_setup_sent, setup, sizeof(hurricane_usb_setup_packet_t));
    
    // Track the operations for our tests
    if (setup->bRequest == USB_REQ_SET_ADDRESS) {
        test_address_set = setup->wValue; // Store the address value being set
    }
    else if (setup->bRequest == USB_REQ_GET_DESCRIPTOR && 
             ((setup->wValue >> 8) == USB_DESC_TYPE_DEVICE)) {
        test_descriptor_requested = 1;
    }
    
    // For GET_DESCRIPTOR requests in tests, simulate successful data
    if (setup->bRequest == USB_REQ_GET_DESCRIPTOR && 
        ((setup->wValue >> 8) == USB_DESC_TYPE_DEVICE)) {
        // Simulate device descriptor for tests
        if (buffer && length >= 18) {
            uint8_t fake_descriptor[18] = {
                18, 1,              // bLength, bDescriptorType
                0x00, 0x02,         // bcdUSB (2.00)
                0, 0, 0, 64,        // bDeviceClass, bDeviceSubclass, bDeviceProtocol, bMaxPacketSize0
                0x5E, 0x04, 0x8E, 0x02, // idVendor, idProduct (Microsoft Xbox controller)
                0x00, 0x01,         // bcdDevice
                1, 2, 3, 1          // iManufacturer, iProduct, iSerialNumber, bNumConfigurations
            };
            memcpy(buffer, fake_descriptor, 18);
            return 18;
        }
    }
    
    // Save any data being sent (for OUT transfers)
    if (buffer && length > 0 && (setup->bmRequestType & 0x80) == 0) {
        size_t copy_len = length < sizeof(last_control_data_sent) ? 
                         length : sizeof(last_control_data_sent);
        memcpy(last_control_data_sent, buffer, copy_len);
        last_control_data_length = copy_len;
    }
    
    return length; // Return success
}

int hurricane_hw_interrupt_in_transfer(uint8_t endpoint, void* buffer, uint16_t length) {
    printf("[stub-hal] hurricane_hw_interrupt_in_transfer(): Endpoint=%u, Length=%u\n", endpoint, length);
    if (buffer && length > 0) {
        // Fill with dummy data for testing
        for (int i = 0; i < length && i < 8; i++) {
            ((uint8_t*)buffer)[i] = i + 0x10; // Fake HID report data
        }
        return 8; // Return a fixed size for testing
    }
    return 0;
}

void hurricane_hw_reset_bus(void) {
    printf("[dummy hal] Bus reset\n");
}