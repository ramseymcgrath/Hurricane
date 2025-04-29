TARGET         = hurricane_proxy
TEST_TARGET    = hurricane_tests
BUILD_DIR      = build

SRC_DIRS       = src core apps/proxy src/hw/common
TEMPLATE_HAL_DIR = src/hw/boards/template
TEENSY_HAL_DIR   = src/hw/boards/teensy41
DUMMY_HAL_DIR    = src/hw/boards/dummy

TEST_SRC_DIR   = tests
INCLUDE_DIRS   = include core src

CC             = gcc
CFLAGS         = -Wall -Wextra -std=c11 -g
CPPFLAGS       = $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -I,$(SRC_DIRS))

SRC_FILES          = $(shell find $(SRC_DIRS) -name '*.c')
TEST_SRC_FILES     = $(shell find $(TEST_SRC_DIR) -name '*.c')
TEMPLATE_HAL_FILES = $(shell find $(TEMPLATE_HAL_DIR) -name '*.c')
TEENSY_HAL_FILES   = $(shell find $(TEENSY_HAL_DIR) -name '*.c')
DUMMY_HAL_FILES    = $(shell find $(DUMMY_HAL_DIR) -name '*.c')

COMMON_OBJ_FILES   = $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)
TEMPLATE_HAL_OBJS  = $(TEMPLATE_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEENSY_HAL_OBJS    = $(TEENSY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
DUMMY_HAL_OBJS     = $(DUMMY_HAL_FILES:%.c=$(BUILD_DIR)/%.o)
TEST_OBJ_FILES     = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean test production build_test build_production

# ====== Targets ======

all: production

production: $(TARGET)

$(TARGET): $(COMMON_OBJ_FILES) $(TEENSY_HAL_OBJS)
	@echo " Linking $@ (production)"
	@$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_TARGET)
	@echo " Running tests..."
	@./$(TEST_TARGET)

$(TEST_TARGET): $(COMMON_OBJ_FILES) $(DUMMY_HAL_OBJS) $(TEST_OBJ_FILES)
	@echo " Linking $@ (tests)"
	@$(CC) $(CFLAGS) $^ -o $@

# ====== Generic object builder ======
$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# ====== Cleanup ======
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)
