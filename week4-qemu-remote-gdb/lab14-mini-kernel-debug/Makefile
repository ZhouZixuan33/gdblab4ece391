LAB := lab14
MODE ?= solution
BUILD := build/$(MODE)

ifeq ($(MODE),solution)
LAB14_SOLUTION := 1
else ifeq ($(MODE),exercise)
LAB14_SOLUTION := 0
else
$(error Unknown MODE '$(MODE)'. Use solution or exercise)
endif

CROSS ?= riscv64-unknown-elf
CC := $(CROSS)-gcc
OBJCOPY ?= $(CROSS)-objcopy
OBJDUMP ?= $(CROSS)-objdump
NM ?= $(CROSS)-nm
READELF ?= $(CROSS)-readelf
QEMU ?= qemu-system-riscv64
GDB ?= gdb-multiarch
INSTALL_HINT := sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make

CFLAGS := -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -mno-relax \
	-msmall-data-limit=0 -g -O0 -ffreestanding -fno-pie -nostdlib \
	-nostartfiles -Wall -Wextra -DLAB14_SOLUTION=$(LAB14_SOLUTION)
KERNEL_LDFLAGS := -T kernel.ld -Wl,--build-id=none
USER_LDFLAGS := -T user.ld -Wl,--build-id=none
QEMU_FLAGS := -machine virt -nographic -bios none -kernel $(BUILD)/kernel.elf

KERNEL_OBJECTS := \
	$(BUILD)/boot.o \
	$(BUILD)/enter_user.o \
	$(BUILD)/trap.o \
	$(BUILD)/kernel.o \
	$(BUILD)/elf_loader.o \
	$(BUILD)/user_image.o

.PHONY: all run debug gdb log check-tools clean symbols disasm user-info check run-check

all: $(BUILD)/kernel.elf $(BUILD)/kernel.bin

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/user.o: user/probe.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/user.elf: $(BUILD)/user.o user.ld
	$(CC) $(CFLAGS) $(USER_LDFLAGS) $< -o $@

$(BUILD)/boot.o: boot.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/enter_user.o: enter_user.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/trap.o: trap.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.o: kernel.c elf_loader.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/elf_loader.o: elf_loader.c elf_loader.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/user_image.o: user_image.S $(BUILD)/user.elf
	$(CC) $(CFLAGS) -DUSER_ELF_PATH=\"$(BUILD)/user.elf\" -c $< -o $@

$(BUILD)/kernel.elf: $(KERNEL_OBJECTS) kernel.ld
	$(CC) $(CFLAGS) $(KERNEL_LDFLAGS) $(KERNEL_OBJECTS) -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

run: $(BUILD)/kernel.elf
	$(QEMU) $(QEMU_FLAGS)

debug: $(BUILD)/kernel.elf
	@echo "QEMU waits for GDB on :1234. In another terminal run: make gdb"
	$(QEMU) $(QEMU_FLAGS) -s -S

gdb: $(BUILD)/kernel.elf
	$(GDB) -q $(BUILD)/kernel.elf \
		-ex "set architecture riscv:rv64" \
		-ex "target remote :1234"

log: $(BUILD)/kernel.elf
	$(QEMU) $(QEMU_FLAGS) -d int,cpu_reset -D $(BUILD)/qemu.log

symbols: $(BUILD)/kernel.elf $(BUILD)/user.elf
	@echo "== kernel symbols"
	$(NM) -n $(BUILD)/kernel.elf
	@echo "== user symbols"
	$(NM) -n $(BUILD)/user.elf

disasm: $(BUILD)/kernel.elf $(BUILD)/user.elf
	$(OBJDUMP) -d $(BUILD)/kernel.elf
	$(OBJDUMP) -d $(BUILD)/user.elf

user-info: $(BUILD)/user.elf
	$(READELF) -h -l $(BUILD)/user.elf

check: all
	MODE=$(MODE) bash check-lab14.sh

run-check: all
	MODE=$(MODE) QEMU=$(QEMU) bash run-check-lab14.sh

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
