#include "hurricane.h"
#if defined(PLATFORM_TEENSY41)
#include "Arduino.h"
#elif defined(PLATFORM_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

void app_main(void)
{
    usb_host_init();

    while (1) {
        usb_host_poll();

#if defined(PLATFORM_TEENSY41)
        delay(100);
#elif defined(PLATFORM_ESP32)
        vTaskDelay(pdMS_TO_TICKS(100));
#endif
    }
}
