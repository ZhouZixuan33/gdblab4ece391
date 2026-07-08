LAB := lab14
SCENARIO ?= hang
BUILD := build/$(SCENARIO)
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

ifeq ($(SCENARIO),good)
SCENARIO_ID := 0
else ifeq ($(SCENARIO),hang)
SCENARIO_ID := 1
else ifeq ($(SCENARIO),wrong-entry)
SCENARIO_ID := 2
else ifeq ($(SCENARIO),reset)
SCENARIO_ID := 3
else
$(error Unknown SCENARIO '$(SCENARIO)'. Use good, hang, wrong-entry, or reset)
endif

CFLAGS := -march=rv64imac -mabi=lp64 -mcmodel=medany -mno-relax -g -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
LDFLAGS := -T linker.ld -Wl,--build-id=none
QEMU_FLAGS := -machine virt -nographic -bios none -kernel $(KERNEL_ELF)

.PHONY: all run debug gdb log check-tools clean symbols disasm scenarios

all: $(KERNEL_ELF) $(KERNEL_BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(START_OBJ): start.S | $(BUILD)
	$(CC) $(CFLAGS) -DSCENARIO_ID=$(SCENARIO_ID) -c $< -o $@

$(KERNEL_ELF): $(START_OBJ) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(START_OBJ) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

scenarios:
	@echo "SCENARIO=good"
	@echo "SCENARIO=hang"
	@echo "SCENARIO=wrong-entry"
	@echo "SCENARIO=reset"

symbols: $(KERNEL_ELF)
	$(NM) -n $(KERNEL_ELF)

disasm: $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF)

run: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS)

debug: $(KERNEL_ELF)
	@echo "QEMU will wait for GDB on :1234 for SCENARIO=$(SCENARIO). In another terminal, run: make SCENARIO=$(SCENARIO) gdb"
	$(QEMU) $(QEMU_FLAGS) -s -S

gdb: $(KERNEL_ELF)
	$(GDB) -q $(KERNEL_ELF) -ex "set architecture riscv:rv64" -ex "target remote :1234"

log: $(KERNEL_ELF)
	$(QEMU) $(QEMU_FLAGS) -d int,cpu_reset -D $(BUILD)/qemu.log
	@echo "QEMU log: $(BUILD)/qemu.log"

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
	rm -rf build
