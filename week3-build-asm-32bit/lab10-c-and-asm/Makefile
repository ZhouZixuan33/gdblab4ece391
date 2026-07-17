CC := riscv64-unknown-elf-gcc
OBJDUMP := riscv64-unknown-elf-objdump
NM := riscv64-unknown-elf-nm
READELF := riscv64-unknown-elf-readelf

# Build 64-bit RISC-V object files for static inspection.
ARCH_FLAGS := -march=rv64im -mabi=lp64

# Common flags used for C and assembly.
#
# -march=rv64im
#   Generate 64-bit RISC-V code with the base integer ISA plus multiply/divide.
#
# -mabi=lp64
#   Use the 64-bit RISC-V integer ABI: long and pointer are 64 bits; int is 32 bits.
#
# -g
#   Include debugging information for objdump/readelf/GDB later.
#
# -O0
#   Keep generated code close to source code for instruction-level reading.
#
# -fno-omit-frame-pointer
#   Keep the frame pointer register easier to inspect in generated assembly.
COMMON_FLAGS := $(ARCH_FLAGS) -g -O0 -fno-omit-frame-pointer

CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -std=c11
ASFLAGS := $(COMMON_FLAGS)
LDFLAGS := $(ARCH_FLAGS) -r

BUILD_DIR := build
MAIN_OBJ := build/main.o
ASM_OBJ := build/asm_helpers.o
COMBINED_OBJ := build/lab10.combined.o
MAIN_ASM := build/main.s
ASM_DUMP := build/asm_helpers.dump
COMBINED_DUMP := build/lab10.combined.dump

.PHONY: all inspect clean

all: $(MAIN_OBJ) $(ASM_OBJ) $(COMBINED_OBJ) $(MAIN_ASM) $(ASM_DUMP) $(COMBINED_DUMP)

# Combine object files without producing a runnable program.
# This lets students inspect how the C object refers to the assembly symbol.
$(COMBINED_OBJ): $(MAIN_OBJ) $(ASM_OBJ)
	$(CC) $(LDFLAGS) build/main.o build/asm_helpers.o -o build/lab10.combined.o

$(MAIN_OBJ): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o build/main.o

$(ASM_OBJ): asm_helpers.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c asm_helpers.S -o build/asm_helpers.o

$(MAIN_ASM): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -S main.c -o build/main.s

$(ASM_DUMP): $(ASM_OBJ)
	$(OBJDUMP) -dr build/asm_helpers.o > build/asm_helpers.dump

$(COMBINED_DUMP): $(COMBINED_OBJ)
	$(OBJDUMP) -dr build/lab10.combined.o > build/lab10.combined.dump

$(BUILD_DIR):
	mkdir -p build

inspect: all
	$(NM) build/main.o build/asm_helpers.o build/lab10.combined.o
	$(READELF) -h build/lab10.combined.o
	$(READELF) -s build/lab10.combined.o
	$(OBJDUMP) -dr build/lab10.combined.o

clean:
	rm -rf build
