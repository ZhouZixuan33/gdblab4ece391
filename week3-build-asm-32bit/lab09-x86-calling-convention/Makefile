CC := gcc
COMMON_FLAGS := -m32 -g -O0 -fno-omit-frame-pointer -fno-pie
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -std=c11
LDFLAGS := -m32 -no-pie
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab09

.PHONY: all run clean

all: $(TARGET)

$(TARGET): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
