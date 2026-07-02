CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -std=c11
BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab08
BROKEN_TARGET := $(BUILD_DIR)/lab08-missing-object
OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/handlers.o
BROKEN_OBJS := $(BUILD_DIR)/main.o

.PHONY: all run objects broken-missing-object clean

all: $(TARGET)

objects: $(OBJS)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(BROKEN_TARGET): $(BROKEN_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(BROKEN_OBJS) -o $(BROKEN_TARGET)

broken-missing-object: $(BROKEN_TARGET)

$(BUILD_DIR)/main.o: main.c handlers.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/handlers.o: handlers.c handlers.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c handlers.c -o $(BUILD_DIR)/handlers.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
