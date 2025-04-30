# === General Setup ===

TEST_TARGET = hurricane_tests
BUILD_DIR   = ../build/tests
SRC_DIRS    = src core src/hw/common
TEST_DIRS   = . unit common
INCLUDE_DIRS = ../include \
               ../lib/hurricane \
               ../lib/hurricane/core \
               ../lib/hurricane/hw \
               ../lib/hurricane/hw/boards/dummy \
               ../lib/hurricane/usb

CC          = gcc
CFLAGS      = -Wall -Wextra -std=c11 -g
CPPFLAGS = $(addprefix -I,$(INCLUDE_DIRS)) -I..

# Optional: Add code coverage flags
COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage

# === Test Sources ===

# Your unit tests
TEST_SRC_FILES = test_runner.c \
                 unit/test_usb_control.c \
                 unit/test_usb_host_controller.c \
                 unit/test_usb_descriptor.c

# Core, USB, and Dummy HAL files
CORE_SRC_FILES = $(wildcard ../lib/hurricane/core/*.c)
USB_SRC_FILES  = $(wildcard ../lib/hurricane/usb/*.c)
DUMMY_HAL_SRC_FILES = $(wildcard ../lib/hurricane/hw/boards/dummy/*.c)

CORE_OBJ_FILES = $(CORE_SRC_FILES:../%.c=$(BUILD_DIR)/../%.o)
USB_OBJ_FILES  = $(USB_SRC_FILES:../%.c=$(BUILD_DIR)/../%.o)
DUMMY_HAL_OBJ_FILES = $(DUMMY_HAL_SRC_FILES:../%.c=$(BUILD_DIR)/../%.o)

TEST_OBJ_FILES = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

ALL_OBJ_FILES = $(CORE_OBJ_FILES) $(USB_OBJ_FILES) $(DUMMY_HAL_OBJ_FILES) $(TEST_OBJ_FILES)

.PHONY: all clean run coverage

# === Default Target ===

all: $(TEST_TARGET)

$(TEST_TARGET): $(ALL_OBJ_FILES)
	@echo " Linking $@"
	@$(CC) $(CFLAGS) $^ -o $@

# === Coverage Target ===

coverage: CFLAGS += $(COVERAGE_FLAGS)
coverage: clean $(TEST_TARGET)
	@echo " Running tests with coverage"
	@./$(TEST_TARGET)
	@echo " Generating coverage report"
	@mkdir -p $(BUILD_DIR)/coverage-report
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors source,unused
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage-report || true
	@echo " Coverage report generated at $(BUILD_DIR)/coverage-report/index.html"

# === Object File Rules ===

$(BUILD_DIR)/%.o: %.c
	@echo " Compiling test source $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/../%.o: ../%.c
	@echo " Compiling project source $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# === Run Target ===

run: $(TEST_TARGET)
	@./$(TEST_TARGET)

# === Clean ===

clean:
	rm -rf $(BUILD_DIR) $(TEST_TARGET)
