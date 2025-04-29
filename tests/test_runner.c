// tests/test_runner.c

#include <stdio.h>
#include "tests/common/test_common.h"

// === Declarations of test suites ===
extern int test_usb_control(void);
extern int test_usb_host_controller(void);
extern int test_usb_descriptor(void);

int main(void)
{
    int failures = 0;

    printf("==== Hurricane Project Unit Tests ====\n");

    failures += test_usb_control();
    failures += test_usb_host_controller();
    failures += test_usb_descriptor();

    printf("\n======================================\n");

    if (failures == 0) {
        printf("✅ All Hurricane tests passed!\n");
        return 0;
    } else {
        printf("❌ %d test(s) failed.\n", failures);
        return 1;
    }
}
