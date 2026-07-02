# Compiler program. Change this if you want to use another C compiler.
CC := gcc

# Flags passed to the compiler:
# -g      include debug information for GDB
# -O0     disable optimization so source and execution match closely
# -Wall   enable common warnings
# -Wextra enable additional warnings
# -std=c11 compile as C11
CFLAGS := -g -O0 -Wall -Wextra -std=c11

# Directory for generated files. Keeping outputs under build/ makes cleanup easy.
BUILD_DIR := build

# Final executable and intermediate object file.
TARGET := $(BUILD_DIR)/lab07
OBJ := $(BUILD_DIR)/main.o

# These are action names, not files that Make should look for.
.PHONY: all run clean

# Default target. Running plain "make" builds the final executable.
all: $(TARGET)

# Link step:
# build/lab07 depends on build/main.o.
# The "| $(BUILD_DIR)" part means the build directory must exist first.
# gcc build/main.o -o build/lab07   # link.
$(TARGET): $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

# Intentional lab bug: config.h is missing from this dependency list.
# Compile step:
# build/main.o depends on main.c, but it should also depend on config.h.
# The "-c" flag means compile only; do not link yet
# gcc -c main.c -o build/main.o     # compile
$(OBJ): main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(OBJ)

# Create the build directory if it does not already exist.
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the executable if needed, then run it.
run: $(TARGET)
	./$(TARGET)

# Remove generated files so the next build starts from scratch.
clean:
	rm -rf $(BUILD_DIR)
