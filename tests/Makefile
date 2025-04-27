TEST_TARGET = hurricane_tests
BUILD_DIR  = ../build/tests
SRC_DIRS   = ../core
TEST_DIRS  = . unit common
INCLUDE_DIRS = ../include $(SRC_DIRS)

CC         = gcc
CFLAGS     = -Wall -Wextra -std=c11 -g
CPPFLAGS   = $(addprefix -I,$(INCLUDE_DIRS)) -I..

# Add coverage flags
COVERAGE_FLAGS = --coverage -fprofile-arcs -ftest-coverage

# We'll use mock implementations for testing instead of real ones
TEST_SRC_FILES = test_runner.c \
                 unit/test_usb_control.c \
                 unit/test_usb_host_controller.c \
                 unit/test_usb_descriptors.c

# Add core files that we want to measure coverage for
CORE_SRC_FILES = $(wildcard ../core/usb/*.c)
CORE_OBJ_FILES = $(CORE_SRC_FILES:../%.c=$(BUILD_DIR)/%.o)

TEST_OBJ_FILES = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run coverage

all: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ_FILES) $(CORE_OBJ_FILES)
	@echo " Linking $@"
	@$(CC) $(CFLAGS) $^ -o $@

coverage: CFLAGS += $(COVERAGE_FLAGS)
coverage: clean $(TEST_TARGET)
	@echo " Running tests with coverage"
	@./$(TEST_TARGET)
	@echo " Generating coverage report"
	@mkdir -p $(BUILD_DIR)/coverage-report
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --ignore-errors source,unused
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage-report || true
	@echo " Coverage report generated at $(BUILD_DIR)/coverage-report/index.html"

$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/core/%.o: ../core/%.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TEST_TARGET)
	@./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TEST_TARGET)