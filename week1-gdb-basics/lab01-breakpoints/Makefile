CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -std=c11
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab01

.PHONY: all run clean

all: $(TARGET)

$(TARGET): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
