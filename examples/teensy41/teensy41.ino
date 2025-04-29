// examples/teensy41/teensy41.ino
#include <usb_host_controller.h>
#include <usb_control.h>
#include <usb_descriptor.h>
#include <usb_hw_hal.h>



void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for Serial to be ready
    }

    Serial.println("Hurricane USB Host: Teensy 4.1 Example Starting...");

    usb_hw_init();      // Low-level USB hardware init
    usb_host_init();    // Core host controller logic init
}

void loop() {
    usb_hw_task();      // Low-level USB traffic handling
    usb_host_poll();    // High-level USB state machine (addressing, configuring, etc.)
}
