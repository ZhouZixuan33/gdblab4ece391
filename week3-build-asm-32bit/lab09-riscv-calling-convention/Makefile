CC := riscv64-unknown-elf-gcc
OBJDUMP := riscv64-unknown-elf-objdump
NM := riscv64-unknown-elf-nm
READELF := riscv64-unknown-elf-readelf

ARCH_FLAGS := -march=rv32im -mabi=ilp32
CFLAGS := $(ARCH_FLAGS) -g -O0 -fno-omit-frame-pointer -Wall -Wextra -std=c11
BUILD_DIR := build
OBJ := build/main.o
ASM := build/main.s
DUMP := build/main.dump

.PHONY: all inspect clean

all: $(OBJ) $(ASM) $(DUMP)

$(OBJ): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o build/main.o

$(ASM): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -S main.c -o build/main.s

$(DUMP): $(OBJ)
	$(OBJDUMP) -dr build/main.o > build/main.dump

$(BUILD_DIR):
	mkdir -p build

inspect: all
	$(NM) build/main.o
	$(READELF) -s build/main.o
	$(OBJDUMP) -dr build/main.o

clean:
	rm -rf build
