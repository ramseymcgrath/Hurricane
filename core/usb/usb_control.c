#include "usb_control.h"
#include <stdio.h>

void usb_handle_setup_packet(const usb_setup_packet_t* setup)
{
    printf("USB SETUP packet:\n");
    printf("  bmRequestType: 0x%02X\n", setup->bmRequestType);
    printf("  bRequest:      0x%02X\n", setup->bRequest);
    printf("  wValue:        0x%04X\n", setup->wValue);
    printf("  wIndex:        0x%04X\n", setup->wIndex);
    printf("  wLength:       %u\n", setup->wLength);
}
