CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -std=c11
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab06
CORE_FILE := $(BUILD_DIR)/lab06.core

.PHONY: all run core gdb-core clean

all: $(TARGET)

$(TARGET): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

core: $(TARGET)
	ulimit -c unlimited; ./$(TARGET)

gdb-core: $(TARGET)
	gdb -q -batch -ex run -ex "generate-core-file $(CORE_FILE)" -ex quit --args ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) core core.*
