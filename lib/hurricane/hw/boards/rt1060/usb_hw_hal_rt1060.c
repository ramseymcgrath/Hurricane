#include "hurricane_hw_hal.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"

void hal_init(void) {
    // Clock and peripheral setup goes here
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
}

void hal_log(const char *msg) {
    PRINTF("[Hurricane] %s\n", msg);
}

uint32_t hal_get_tick(void) {
    return SDK_GetTicks(); // or SysTick
}

// Stubs
bool hal_usb_device_ready(void) {
    return false;
}

bool hal_usb_host_ready(void) {
    return false;
}
