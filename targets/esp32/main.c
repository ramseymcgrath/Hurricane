#include "hurricane.h"
#include "hurricane_hw_hal.h"

void app_main(void) {
    hal_init();
    hal_log("🐍 Hurricane USB stack booted on ESP32");

    while (1) {
        hurricane_poll();
        vTaskDelay(1);  // Minimal delay to yield RTOS
    }
}
