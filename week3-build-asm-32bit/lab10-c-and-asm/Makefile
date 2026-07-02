CC := gcc
COMMON_FLAGS := -m32 -g -O0 -fno-omit-frame-pointer -fno-pie
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -std=c11
ASFLAGS := $(COMMON_FLAGS)
LDFLAGS := -m32 -no-pie
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab10
OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/asm_helpers.o

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/asm_helpers.o: asm_helpers.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
