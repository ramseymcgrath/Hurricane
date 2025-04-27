// tests/test_usb_control.c

#include "core/usb/usb_control.h"
#include <stdio.h>

int test_usb_control(void)
{
    int failures = 0;

    usb_setup_packet_t pkt = {
        .bmRequestType = 0x80,
        .bRequest = 0x06,
        .wValue = 0x0100,
        .wIndex = 0,
        .wLength = 18
    };

    if (pkt.bRequest != 0x06) {
        printf("FAIL: bRequest mismatch\n");
        failures++;
    }

    if (pkt.wValue != 0x0100) {
        printf("FAIL: wValue mismatch\n");
        failures++;
    }

    if (pkt.wLength != 18) {
        printf("FAIL: wLength mismatch\n");
        failures++;
    }

    return failures;
}

