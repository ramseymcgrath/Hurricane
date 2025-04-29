TARGET         = hurricane_proxy
TEST_TARGET    = hurricane_tests
BUILD_DIR      = build

# === Source dirs ===
CORE_DIR        = core
USB_DIR         = src/usb
PROXY_DIR       = apps/proxy
TEST_DIR        = tests
DUMMY_HAL_DIR   = src/hw/boards/dummy
TEENSY_HAL_DIR  = src/hw/boards/teensy41

INCLUDE_DIRS    = include core src

CC             = gcc
CFLAGS         = -Wall -Wextra -std=c11 -g
CPPFLAGS       = $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -I,$(CORE_DIR) $(USB_DIR) $(PROXY_DIR))

# === Source files ===

# Core code (shared between production and test)
CORE_SRC_FILES  = $(shell find $(CORE_DIR) -name '*.c')
USB_SRC_FILES   = $(shell find $(USB_DIR) -name '*.c')
PROXY_SRC_FILES = $(shell find $(PROXY_DIR) -name '*.c')

# HALs
DUMMY_HAL_FILES = $(shell find $(DUMMY_HAL_DIR) -name '*.c')
TEENSY_HAL_FILES = $(shell find $(TEENSY_HAL_DIR) -name '*.c')

# Tests
TEST_SRC_FILES  = $(shell find $(TEST_DIR) -name '*.c')

# === Object files ===

COMMON_OBJ_FILES = $(CORE_SRC_FILES:%.c=$(BUILD_DIR)/%.o) $(USB_SRC_FILES:%.c=$(BUILD_DIR)/%.o) $(PROXY_SRC_FILES:%.c=$(BUILD_DIR)/%.o)
DUMMY_HAL_OBJS   = $(DUMMY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEENSY_HAL_OBJS  = $(TEENSY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEST_OBJ_FILES   = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean test production run_tests coverage

all: production

# === Production build ===
production: $(TARGET)

$(TARGET): $(COMMON_OBJ_FILES) $(TEENSY_HAL_OBJS)
	@echo " Linking $@ (production)"
	@$(CC) $(CFLAGS) $^ -o $@

# === Test build ===
test: $(TEST_TARGET)

$(TEST_TARGET): $(COMMON_OBJ_FILES) $(DUMMY_HAL_OBJS) $(TEST_OBJ_FILES)
	@echo " Linking $@ (tests)"
	@$(CC) $(CFLAGS) $^ -o $@

run_tests: test
	@./$(TEST_TARGET)

coverage: clean
	@$(MAKE) CFLAGS="$(CFLAGS) --coverage -fprofile-arcs -ftest-coverage" test
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
