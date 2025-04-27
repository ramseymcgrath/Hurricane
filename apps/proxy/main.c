#include "usb/usb_control.h"

int main()
{
    usb_setup_packet_t dummy_setup = {
        .bmRequestType = 0x80,
        .bRequest = 0x06,
        .wValue = (1 << 8),
        .wIndex = 0,
        .wLength = 18,
    };

    usb_handle_setup_packet(&dummy_setup);
    return 0;
}
