# === Hurricane Project Makefile ===
# Usage:
#   make BOARD=<board> [target]
# Boards: dummy (default), teensy41, esp32, rt1060, max3421e

TARGET        = hurricane_app
TEST_TARGET   = hurricane_tests
BUILD_DIR     = build

# NXP SDK path for RT1060 and LPC55S69 targets
NXP_SDK_PATH   = /Users/ramseymcgrath/code/mcuxpresso-sdk/mcuxsdk

# === Source Dirs ===
HURRICANE_DIR = lib/hurricane
CORE_DIR      = $(HURRICANE_DIR)/core
USB_DIR       = $(HURRICANE_DIR)/usb
HW_DIR        = $(HURRICANE_DIR)/hw
BOARDS_DIR    = $(HW_DIR)/boards
TEST_DIR      = test

INCLUDE_DIRS  = $(HURRICANE_DIR) $(CORE_DIR) $(USB_DIR) $(HW_DIR)

CC            = gcc
CFLAGS        = -Wall -Wextra -std=c11 -g
CPPFLAGS      = $(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS       =

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

# === Board Selection ===
BOARD ?= dummy

ifeq ($(BOARD),dummy)
  HAL_DIR = $(BOARDS_DIR)/dummy
  HAL_FILES = $(wildcard $(HAL_DIR)/*.c)
else ifeq ($(BOARD),teensy41)
  HAL_DIR = $(BOARDS_DIR)/teensy41
  HAL_FILES = $(wildcard $(HAL_DIR)/*.c)
else ifeq ($(BOARD),esp32)
  HAL_DIR = $(BOARDS_DIR)/esp32
  HAL_FILES = $(wildcard $(HAL_DIR)/*.c)
else ifeq ($(BOARD),rt1060)
  HAL_DIR = $(BOARDS_DIR)/rt1060
  HAL_FILES = $(wildcard $(HAL_DIR)/*.c)
else ifeq ($(BOARD),max3421e)
  HAL_DIR = $(BOARDS_DIR)/max3421e
  HAL_FILES = $(wildcard $(HAL_DIR)/*.c)
else
  $(error Unknown BOARD '$(BOARD)')
endif

# === Source Files ===
CORE_SRC_FILES = $(wildcard $(CORE_DIR)/*.c)
USB_SRC_FILES  = $(wildcard $(USB_DIR)/*.c)
HW_FILES       = $(wildcard $(HW_DIR)/*.c)
SRC_FILES      = $(CORE_SRC_FILES) $(USB_SRC_FILES) $(HW_FILES) $(HAL_FILES)

# === Object Files ===
OBJ_FILES = $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)

# === Targets ===
.PHONY: all clean test production run_tests coverage build_rt1060 build_lpc55s69

all: production

production: $(TARGET)

$(TARGET): $(OBJ_FILES) src/main.c
	@echo " Linking $@ (production for $(BOARD))"
	$(Q)$(CC) $(CFLAGS) $(OBJ_FILES) src/main.c -o $@

test: BOARD=dummy

test: $(OBJ_FILES) $(TEST_DIR)/test_runner.c $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/common/*.c)
	@echo " Linking $@ (tests)"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJ_FILES) $(TEST_DIR)/test_runner.c $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/common/*.c) -o $(TEST_TARGET)

# === CMake-based targets using the NXP SDK ===
build_rt1060:
	@echo "Building RT1060 target with NXP SDK..."
	@mkdir -p build_rt1060
	@cd build_rt1060 && cmake -DNXP_SDK_PATH=$(NXP_SDK_PATH) -DHURRICANE_TARGET_DEVICE=MIMXRT1062 ..
	@cmake --build build_rt1060

build_lpc55s69:
	@echo "Building LPC55S69 target with NXP SDK..."
	@mkdir -p build_lpc55s69
	@cd build_lpc55s69 && cmake -DNXP_SDK_PATH=$(NXP_SDK_PATH) -DHURRICANE_TARGET_DEVICE=LPC55S69 ..
	@cmake --build build_lpc55s69

run_tests: test
	@./$(TEST_TARGET)

coverage: clean
	@$(MAKE) CFLAGS="$(CFLAGS) --coverage -fprofile-arcs -ftest-coverage" LDFLAGS="$(LDFLAGS) --coverage" test
	@./$(TEST_TARGET)
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors source
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage-report || true

# === Compilation Rule ===
$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET) build_rt1060 build_lpc55s69
