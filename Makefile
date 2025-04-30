TEST_TARGET    = hurricane_tests
BUILD_DIR      = build

# === Source dirs ===
HURRICANE_DIR   = lib/hurricane
CORE_DIR        = $(HURRICANE_DIR)/core
USB_DIR         = $(HURRICANE_DIR)/usb
HW_DIR          = $(HURRICANE_DIR)/hw
DUMMY_HAL_DIR   = $(HURRICANE_DIR)/hw/boards/dummy
TEENSY_HAL_DIR  = $(HURRICANE_DIR)/hw/boards/teensy41
ESP32_HAL_DIR   = $(HURRICANE_DIR)/hw/boards/esp32-s3-devkitc-1
TEST_DIR        = test

INCLUDE_DIRS    = $(HURRICANE_DIR) $(CORE_DIR) $(USB_DIR) $(HW_DIR)

CC             = gcc
CFLAGS         = -Wall -Wextra -std=c11 -g
CPPFLAGS       = $(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS        = 

# Detect macOS Apple Silicon and use clang
ifeq ($(shell uname -s),Darwin)
  ifeq ($(shell uname -m),arm64)
    CC = clang
    # For troubleshooting
    VERBOSE = 1
  endif
endif

# For verbose output
ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

# macOS's clang integrates coverage libraries by default when using --coverage

# === Source files ===
CORE_SRC_FILES    = $(shell find $(CORE_DIR) -name '*.c')
USB_SRC_FILES     = $(shell find $(USB_DIR) -name '*.c')
HW_FILES          = $(shell find $(HW_DIR) -type f -name '*.c' -not -path "*/boards/*")
DUMMY_HAL_FILES   = $(shell find $(DUMMY_HAL_DIR) -name '*.c')
TEENSY_HAL_FILES  = $(shell find $(TEENSY_HAL_DIR) -name '*.c')
ESP32_HAL_FILES   = $(shell find $(ESP32_HAL_DIR) -name '*.c')
TEST_SRC_FILES    = $(shell find $(TEST_DIR) -name '*.c')

# === Object files ===
COMMON_OBJ_FILES  = $(CORE_SRC_FILES:%.c=$(BUILD_DIR)/%.o) \
                    $(USB_SRC_FILES:%.c=$(BUILD_DIR)/%.o) \
                    $(HW_FILES:%.c=$(BUILD_DIR)/%.o)

DUMMY_HAL_OBJS    = $(DUMMY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEENSY_HAL_OBJS   = $(TEENSY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
ESP32_HAL_OBJS    = $(ESP32_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEST_OBJ_FILES    = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean test production run_tests coverage

all: production

# === Production build ===
production: $(TARGET)

$(TARGET): $(COMMON_OBJ_FILES) $(TEENSY_HAL_OBJS)
	@echo " Linking $@ (production)"
	@$(CC) $(CFLAGS) $^ -o $@

# === Test build ===
test: $(COMMON_OBJ_FILES) $(DUMMY_HAL_OBJS) $(TEST_OBJ_FILES)
	@echo " Linking $@ (tests)"
	$(Q)$(CC) $(CFLAGS) $^ -o $(TEST_TARGET)

run_tests: test
	@./$(TEST_TARGET)

coverage: clean
	@$(MAKE) CFLAGS="$(CFLAGS) --coverage -fprofile-arcs -ftest-coverage" LDFLAGS="$(LDFLAGS) $(RUNTIME_COV_LIB) --coverage" test
	@./$(TEST_TARGET)
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors source
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage-report || true

# === Generic compilation rule ===
$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)
