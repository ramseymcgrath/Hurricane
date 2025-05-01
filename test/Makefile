# === Hurricane Test Makefile ===
# Usage:
#   make [target]
# Always uses the dummy HAL for tests

TEST_TARGET = hurricane_tests
BUILD_DIR   = ../build/tests

# === Source Dirs ===
HURRICANE_DIR = ../lib/hurricane
CORE_DIR      = $(HURRICANE_DIR)/core
USB_DIR       = $(HURRICANE_DIR)/usb
HW_DIR        = $(HURRICANE_DIR)/hw
DUMMY_HAL_DIR = $(HW_DIR)/boards/dummy
TEST_DIR      = .
UNIT_TEST_DIR = unit
COMMON_TEST_DIR = common

INCLUDE_DIRS = $(HURRICANE_DIR) $(CORE_DIR) $(USB_DIR) $(HW_DIR) $(DUMMY_HAL_DIR)

CC          = gcc
CFLAGS      = -Wall -Wextra -std=c11 -g
CPPFLAGS    = $(addprefix -I,$(INCLUDE_DIRS)) -I..

# Detect macOS Apple Silicon and use clang
ifeq ($(shell uname -s),Darwin)
  ifeq ($(shell uname -m),arm64)
    CC = clang
    VERBOSE = 1
  endif
endif

ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

COVERAGE_FLAGS = --coverage

# === Source Files ===
CORE_SRC_FILES      = $(wildcard $(CORE_DIR)/*.c)
USB_SRC_FILES       = $(wildcard $(USB_DIR)/*.c)
DUMMY_HAL_SRC_FILES = $(wildcard $(DUMMY_HAL_DIR)/*.c)
TEST_SRC_FILES      = test_runner.c $(wildcard $(UNIT_TEST_DIR)/*.c) $(wildcard $(COMMON_TEST_DIR)/*.c)

ALL_SRC_FILES = $(CORE_SRC_FILES) $(USB_SRC_FILES) $(DUMMY_HAL_SRC_FILES) $(addprefix $(TEST_DIR)/,$(TEST_SRC_FILES))

OBJ_FILES = $(ALL_SRC_FILES:../%.c=$(BUILD_DIR)/../%.o)
OBJ_FILES += $(ALL_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run coverage

all: $(TEST_TARGET)

$(TEST_TARGET): $(OBJ_FILES)
	@echo " Linking $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

coverage: CFLAGS += $(COVERAGE_FLAGS)
coverage: LDFLAGS += --coverage
coverage: clean $(TEST_TARGET)
	@echo " Running tests with coverage"
	@./$(TEST_TARGET)
	@echo " Generating coverage report"
	@mkdir -p $(BUILD_DIR)/coverage-report
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors source,unused
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage-report || true
	@echo " Coverage report generated at $(BUILD_DIR)/coverage-report/index.html"

$(BUILD_DIR)/%.o: %.c
	@echo " Compiling test source $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/../%.o: ../%.c
	@echo " Compiling project source $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TEST_TARGET)
	@./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TEST_TARGET)
