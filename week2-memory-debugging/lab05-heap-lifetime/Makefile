CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -std=c11
ASAN_CFLAGS := $(CFLAGS) -fsanitize=address -fno-omit-frame-pointer
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab05
ASAN_TARGET := $(BUILD_DIR)/lab05-asan

.PHONY: all run asan asan-build clean

all: $(TARGET)

$(TARGET): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(ASAN_TARGET): main.c | $(BUILD_DIR)
	$(CC) $(ASAN_CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

asan-build: $(ASAN_TARGET)

asan: $(ASAN_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(ASAN_TARGET)

clean:
	rm -rf $(BUILD_DIR)
