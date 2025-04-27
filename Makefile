TARGET     = hurricane_proxy
TEST_TARGET = hurricane_tests
BUILD_DIR  = build
SRC_DIRS   = core apps/proxy hw/common hw/boards/template
TEST_SRC_DIR = tests
INCLUDE_DIRS = include core

CC         = gcc
CFLAGS     = -Wall -Wextra -std=c11 -g
CPPFLAGS   = $(addprefix -I,$(INCLUDE_DIRS)) $(addprefix -I,$(SRC_DIRS))

SRC_FILES  = $(shell find $(SRC_DIRS) -name '*.c')
TEST_SRC_FILES = $(shell find $(TEST_SRC_DIR) -name '*.c')
OBJ_FILES  = $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)
TEST_OBJ_FILES = $(TEST_SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@echo " Linking $@"
	@$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	@./$(TEST_TARGET)

$(TEST_TARGET): $(OBJ_FILES) $(TEST_OBJ_FILES)
	@echo " Linking $@ (tests)"
	@$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)
