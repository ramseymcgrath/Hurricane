TARGET     = hurricane_proxy
BUILD_DIR  = build
SRC_DIRS   = core apps/proxy hw/common
INCLUDE_DIRS = include

CC         = gcc
CFLAGS     = -Wall -Wextra -std=c11 -g
CPPFLAGS   = $(addprefix -I,$(INCLUDE_DIRS))

SRC_FILES  = $(shell find $(SRC_DIRS) -name '*.c')
OBJ_FILES  = $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	@echo " Linking $@"
	@$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	@echo " Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
