MODE ?= solution
BUILD := build/$(MODE)
CROSS ?= riscv64-unknown-elf
CC := $(CROSS)-gcc
NM := $(CROSS)-nm
QEMU ?= qemu-system-riscv64
ifeq ($(MODE),solution)
SOLUTION := 1
else ifeq ($(MODE),exercise)
SOLUTION := 0
else
$(error MODE must be solution or exercise)
endif
CFLAGS := -I../lab17-scheduler-lifecycle -march=rv64imac_zicsr -mabi=lp64 \
 -mcmodel=medany -mno-relax -msmall-data-limit=0 -g -O0 -ffreestanding \
 -fno-pie -nostdlib -nostartfiles -Wall -Wextra -DLAB18_SOLUTION=$(SOLUTION)
OBJS := $(BUILD)/boot.o $(BUILD)/switch.o $(BUILD)/kernel.o
.PHONY: all run check run-check clean
all: $(BUILD)/kernel.elf
$(BUILD): ; mkdir -p $@
$(BUILD)/boot.o: boot.S | $(BUILD) ; $(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/switch.o: ../lab17-scheduler-lifecycle/switch.S ../lab17-scheduler-lifecycle/thread.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.o: kernel.c ../lab17-scheduler-lifecycle/thread.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(OBJS) kernel.ld
	$(CC) $(CFLAGS) -T kernel.ld -Wl,--build-id=none $(OBJS) -o $@
run: all ; $(QEMU) -machine virt -nographic -bios none -kernel $(BUILD)/kernel.elf
check: all ; MODE=$(MODE) bash check-lab18.sh
run-check: all ; MODE=$(MODE) QEMU=$(QEMU) bash run-check-lab18.sh
clean: ; rm -rf build
