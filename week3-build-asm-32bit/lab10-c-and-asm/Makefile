CC := gcc

# COMMON_FLAGS are shared by both C compilation and assembly compilation.
#
# Flag notes:
# -m32
#   Build 32-bit x86 code instead of the host machine's default 64-bit code.
#   This lab studies 32-bit registers such as eax, ebx, ecx, edx, esp, and ebp.
#
# -g
#   Include debugging information so GDB can show source lines, function names,
#   variables, and useful symbols.
#
# -O0
#   Turn off optimization. This keeps the generated code closer to the source
#   code and makes instruction-by-instruction debugging easier for beginners.
#
# -fno-omit-frame-pointer
#   Keep ebp as a frame pointer. This makes stack frames easier to inspect in
#   GDB because arguments and local variables have stable offsets from ebp.
#
# -fno-pie
#   During compilation, do not generate position-independent executable code.
#   This keeps addresses and disassembly simpler for this lab.
#
# Expanded value:
# -m32 -g -O0 -fno-omit-frame-pointer -fno-pie
COMMON_FLAGS := -m32 -g -O0 -fno-omit-frame-pointer -fno-pie

# CFLAGS are used when compiling main.c into build/main.o.
#
# Extra C-only warning and language flags:
# -Wall
#   Enable a useful baseline set of compiler warnings.
#
# -Wextra
#   Enable additional warnings beyond -Wall.
#
# -std=c11
#   Compile the C file using the C11 language standard.
#
# Expanded value:
# -m32 -g -O0 -fno-omit-frame-pointer -fno-pie -Wall -Wextra -std=c11
CFLAGS := $(COMMON_FLAGS) -Wall -Wextra -std=c11

# ASFLAGS are used when compiling asm_helpers.S into build/asm_helpers.o.
# Expanded value:
# -m32 -g -O0 -fno-omit-frame-pointer -fno-pie
ASFLAGS := $(COMMON_FLAGS)

# LDFLAGS are used when linking object files into the final executable.
#
# Linker flag notes:
# -m32
#   Link a 32-bit executable. This must match the 32-bit object files produced
#   during compilation.
#
# -no-pie
#   During linking, produce a non-PIE executable. This pairs with -fno-pie and
#   keeps addresses easier to understand in GDB.
#
# Expanded value:
# -m32 -no-pie
LDFLAGS := -m32 -no-pie

BUILD_DIR := build
TARGET := $(BUILD_DIR)/lab10
OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/asm_helpers.o

.PHONY: all run clean

all: $(TARGET)

# Build the final executable from both object files.
#
# Fully expanded command:
# gcc -m32 -no-pie build/main.o build/asm_helpers.o -o build/lab10
#
# The part after "|" means build/ must exist before linking, but changing the
# timestamp of build/ alone should not force relinking.
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Compile the C source file into an object file.
#
# Fully expanded command:
# gcc -m32 -g -O0 -fno-omit-frame-pointer -fno-pie -Wall -Wextra -std=c11 -c main.c -o build/main.o
$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

# Compile the assembly source file into an object file.
#
# Fully expanded command:
# gcc -m32 -g -O0 -fno-omit-frame-pointer -fno-pie -c asm_helpers.S -o build/asm_helpers.o
$(BUILD_DIR)/asm_helpers.o: asm_helpers.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c asm_helpers.S -o $(BUILD_DIR)/asm_helpers.o

# Create the build directory if it does not already exist.
#
# Fully expanded command:
# mkdir -p build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the executable first, then run it.
#
# Fully expanded command:
# ./build/lab10
run: $(TARGET)
	./$(TARGET)

# Remove generated files.
#
# Fully expanded command:
# rm -rf build
clean:
	rm -rf $(BUILD_DIR)
