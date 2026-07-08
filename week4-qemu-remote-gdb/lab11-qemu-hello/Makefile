LAB := lab11
BUILD := build
START_OBJ := $(BUILD)/start.o
KERNEL_ELF := $(BUILD)/kernel.elf
KERNEL_BIN := $(BUILD)/kernel.bin

CROSS ?= riscv64-unknown-elf
CC := $(CROSS)-gcc
OBJCOPY ?= $(CROSS)-objcopy
OBJDUMP ?= $(CROSS)-objdump
NM ?= $(CROSS)-nm
READELF ?= $(CROSS)-readelf
QEMU ?= qemu-system-riscv64
GDB ?= gdb-multiarch
INSTALL_HINT := sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make

CFLAGS := -march=rv64imac -mabi=lp64 -mcmodel=medany -mno-relax -g -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
LDFLAGS := -T linker.ld -Wl,--build-id=none
QEMU_FLAGS := -machine virt -nographic -bios none -kernel $(KERNEL_ELF)

.PHONY: all artifacts inspect run debug gdb log check-tools clean

all: $(KERNEL_ELF) $(KERNEL_BIN)

artifacts:
	@echo "Lab 11 artifact map:"
	@echo "  start.S       -> $(START_OBJ)"
	@echo "    RISC-V assembly object file."
	@echo "  $(START_OBJ) + linker.ld -> $(KERNEL_ELF)"
	@echo "    RISC-V ELF. QEMU loads this and GDB reads its symbols."
	@echo "  $(KERNEL_ELF) -> $(KERNEL_BIN)"
	@echo "    Flat bytes extracted for comparison; Lab 11 boots the ELF."

inspect: $(KERNEL_ELF) $(KERNEL_BIN)
	@echo "== file types"
	file $(START_OBJ) $(KERNEL_ELF) $(KERNEL_BIN)
	@echo
	@echo "== ELF header"
	$(READELF) -h $(KERNEL_ELF)
	@echo
	@echo "== symbols"
	$(NM) -n $(KERNEL_ELF)
	@echo
	@echo "== first instructions"
	$(OBJDUMP) -d $(KERNEL_ELF) | sed -n '1,80p'

$(BUILD):
	mkdir -p $(BUILD)

$(START_OBJ): start.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(START_OBJ) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(START_OBJ) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

run: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS)

debug: $(KERNEL_ELF)
	@echo "QEMU will wait for GDB on :1234. In another terminal, run: make gdb"
	$(QEMU) $(QEMU_FLAGS) -s -S

gdb: $(KERNEL_ELF)
	$(GDB) -q $(KERNEL_ELF) -ex "set architecture riscv:rv64" -ex "target remote :1234"

log: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS) -d int,cpu_reset -D $(BUILD)/qemu.log

check-tools:
	@missing=0; \
	for tool in "$(CC)" "$(OBJCOPY)" "$(OBJDUMP)" "$(NM)" "$(READELF)" "$(QEMU)"; do \
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
