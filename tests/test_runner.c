// tests/test_runner.c

#include <stdio.h>
#include "common/test_common.h"

// Test function declarations
extern int test_usb_control(void);
extern int test_usb_host_controller(void);
extern int test_usb_descriptors(void);

int main(void)
{
    int failures = 0;

    printf("Running Hurricane tests...\n");

    // Run all test suites
    failures += test_usb_control();
    failures += test_usb_host_controller();
    failures += test_usb_descriptors();

    // Print test summary
    if (failures == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    } else {
        printf("\n%d test(s) failed.\n", failures);
        return 1;
    }
}

