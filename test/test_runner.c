// tests/test_runner.c

#include <stdio.h>
#include "common/test_common.h"
#define PLATFORM_DUMMY
// === Declarations of test suites ===
extern int test_usb_control(void);
extern int test_usb_host_controller(void);
extern int test_usb_descriptor(void);
extern int test_usb_interface_manager(void);

int main(void)
{
    int failures = 0;

    printf("==== Hurricane Project Unit Tests ====\n");

    failures += test_usb_control();
    failures += test_usb_host_controller();
    failures += test_usb_descriptor();
    failures += test_usb_interface_manager();

    printf("\n======================================\n");

    if (failures == 0) {
        printf("✅ All Hurricane tests passed!\n");
        return 0;
    } else {
        printf("❌ %d test(s) failed.\n", failures);
        return 1;
    }
}
