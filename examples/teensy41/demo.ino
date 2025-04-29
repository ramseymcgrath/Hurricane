#include "hurricane/core/usb_host_controller.h"
#include "hurricane/hw/usb_hw_hal.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) ;
    usb_host_init();
}

void loop() {
    usb_host_poll();
    usb_hw_task();
    delay(5); // Give USB stack time to breathe
}
