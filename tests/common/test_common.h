#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdbool.h>

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s (line %d): %s\n", __FILE__, __LINE__, message); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s (line %d): %s - Expected %d, got %d\n", \
                   __FILE__, __LINE__, message, (expected), (actual)); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("FAIL: %s (line %d): %s - Expected \"%s\", got \"%s\"\n", \
                   __FILE__, __LINE__, message, (expected), (actual)); \
            return 1; \
        } \
    } while (0)

#define TEST_FAIL(message) \
    do { \
        printf("FAIL: %s (line %d): %s\n", __FILE__, __LINE__, message); \
        return 1; \
    } while (0)

#define TEST_PASS() \
    do { \
        return 0; \
    } while (0)

#define RUN_TEST(test_function) \
    do { \
        printf("Running %s...\n", #test_function); \
        int result = test_function(); \
        if (result == 0) { \
            printf("PASS: %s\n", #test_function); \
        } \
        failures += result; \
    } while (0)

#endif /* TEST_COMMON_H */