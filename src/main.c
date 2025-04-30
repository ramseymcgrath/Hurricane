#include "hurricane.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    usb_host_init();

    while (1) {
        usb_host_poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}