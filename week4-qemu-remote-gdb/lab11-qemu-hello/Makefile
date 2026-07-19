LAB := lab11
BUILD := build

START_OBJ := $(BUILD)/start.o
MAIN_OBJ := $(BUILD)/main.o
ASM_HELPERS_OBJ := $(BUILD)/asm_helpers.o
OBJECTS := $(START_OBJ) $(MAIN_OBJ) $(ASM_HELPERS_OBJ)

KERNEL_ELF := $(BUILD)/kernel.elf

CROSS ?= riscv64-unknown-elf
CC := $(CROSS)-gcc
OBJDUMP ?= $(CROSS)-objdump
NM ?= $(CROSS)-nm
READELF ?= $(CROSS)-readelf
QEMU ?= qemu-system-riscv64
GDB ?= gdb-multiarch
INSTALL_HINT := sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make

ARCH_FLAGS := -march=rv64imac -mabi=lp64 -mcmodel=medany -mno-relax -msmall-data-limit=0
COMMON_FLAGS := $(ARCH_FLAGS) -g -O0 -ffreestanding -fno-builtin -fno-omit-frame-pointer -Wall -Wextra
CFLAGS := $(COMMON_FLAGS) -std=c11
ASFLAGS := $(COMMON_FLAGS)
LDFLAGS := $(ARCH_FLAGS) -nostdlib -nostartfiles -T linker.ld -Wl,--build-id=none
QEMU_FLAGS := -machine virt -nographic -bios none -kernel $(KERNEL_ELF)

.PHONY: all artifacts inspect run debug gdb log check-tools clean

all: $(KERNEL_ELF)

artifacts:
	@echo "Lab 11 runtime artifact map:"
	@echo "  start.S           -> $(START_OBJ)"
	@echo "    Tiny entry point: set sp, call kernel_entry, then wait in halt_loop."
	@echo "  main.c            -> $(MAIN_OBJ)"
	@echo "    Runtime driver: calls asm_add_three and check_preserves_s1."
	@echo "  asm_helpers.S     -> $(ASM_HELPERS_OBJ)"
	@echo "    Hand-written RV64 helper functions from the Lab 10 concept."
	@echo "  $(OBJECTS) + linker.ld -> $(KERNEL_ELF)"
	@echo "    RISC-V ELF. QEMU loads this and GDB reads its symbols."

inspect: $(KERNEL_ELF)
	@echo "== file types"
	file $(OBJECTS) $(KERNEL_ELF)
	@echo
	@echo "== ELF header"
	$(READELF) -h $(KERNEL_ELF)
	@echo
	@echo "== symbols"
	$(NM) -n $(KERNEL_ELF)
	@echo
	@echo "== selected runtime disassembly"
	$(OBJDUMP) -d $(KERNEL_ELF) --disassemble=kernel_entry
	@echo
	$(OBJDUMP) -d $(KERNEL_ELF) --disassemble=asm_add_three
	@echo
	$(OBJDUMP) -d $(KERNEL_ELF) --disassemble=check_preserves_s1
	@echo
	$(OBJDUMP) -d $(KERNEL_ELF) --disassemble=broken_helper

$(BUILD):
	mkdir -p $(BUILD)

$(START_OBJ): start.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(MAIN_OBJ): main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_HELPERS_OBJ): asm_helpers.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

run: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS)

debug: $(KERNEL_ELF)
	@echo "QEMU will wait for GDB on :1234. In another terminal, run the manual GDB commands from README.md."
	$(QEMU) $(QEMU_FLAGS) -s -S

gdb: $(KERNEL_ELF)
	$(GDB) -q $(KERNEL_ELF) -ex "set architecture riscv:rv64" -ex "target remote :1234"

log: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS) -d int,cpu_reset -D $(BUILD)/qemu.log

check-tools:
	@missing=0; \
	for tool in "$(CC)" "$(OBJDUMP)" "$(NM)" "$(READELF)" "$(QEMU)"; do \
		if ! command -v "$$tool" >/dev/null; then \
			echo "Missing $$tool"; \
			missing=1; \
		fi; \
	done; \
	if ! command -v "$(GDB)" >/dev/null && ! command -v gdb >/dev/null; then \
		echo "Missing $(GDB) or gdb"; \
		missing=1; \
	fi; \
	if [ "$$missing" -ne 0 ]; then \
		echo; \
		echo "Install Week 4 tools with:"; \
		echo "  $(INSTALL_HINT)"; \
		exit 1; \
	fi; \
	echo "Week 4 tool check passed for $(LAB)."

clean:
	rm -rf $(BUILD)
