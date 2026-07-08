# Lab 12: RISC-V Remote Breakpoints, Symbols, and Raw Addresses

## Goal

Use symbols and raw addresses to stop a RISC-V QEMU target at the code you care about.

You will practice:

```gdb
set architecture riscv:rv64
target remote :1234
info functions
break kernel_entry
break debug_checkpoint
break *0x80000000
```

## Failure Scenario

The target runs quickly and prints several checkpoint lines. If you only look at output, you cannot prove exactly when each function ran.

Your job is to stop the remote target at:

- `_start`
- `kernel_entry`
- `debug_checkpoint`
- the raw entry address, usually `0x80000000` in this lab

## Concept Warmup

### Symbol File / symbol file

GDB needs the ELF file because the ELF contains names and debug information.

In this lab:

```text
build/kernel.elf
  QEMU loads this with -kernel
  GDB reads symbols from this
```

If GDB does not know `kernel_entry`, confirm you started GDB with:

```bash
gdb-multiarch build/kernel.elf
```

### Raw Address / raw address

Sometimes symbols are missing or suspicious. Then you can break at an exact instruction address:

```gdb
break *0x80000000
```

This is common in early kernel debugging because symbol files, linker scripts, and load addresses must agree.

## Guided Mode

Step 1: Build the target.

```bash
make
```

Step 2: Inspect symbols.

```bash
make symbols
```

What to look for: `_start` should be near `80000000`.

Step 3: Inspect disassembly.

```bash
make disasm
```

What to look for: find the first instructions under `_start` and `kernel_entry`.

Step 4: Start QEMU paused.

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf -s -S
```

Step 5: In a second terminal, connect GDB manually.

```bash
gdb-multiarch build/kernel.elf
```

```gdb
set architecture riscv:rv64
target remote :1234
```

Step 6: Confirm GDB knows the symbols.

```gdb
info files
info functions
```

What to look for: `_start`, `kernel_entry`, `init_console`, `debug_checkpoint`, and `scheduler_checkpoint`.

Step 7: Break by function name.

```gdb
break kernel_entry
break debug_checkpoint
info breakpoints
continue
```

Step 8: Continue to the next symbolic breakpoint.

```gdb
continue
info registers pc sp ra
x/i $pc
```

Step 9: Try a raw address breakpoint.

Restart QEMU, then in GDB:

```gdb
delete
break *0x80000000
continue
```

What to look for: this stops at the linked entry address even if you do not rely on the function name.

After you understand the manual commands, the Makefile shortcuts are:

```bash
make debug
make gdb
```

## Review Questions

1. Which file does QEMU load in this lab?

   Answer: `build/kernel.elf`.

2. Which file gives GDB function names?

   Answer: `build/kernel.elf`.

3. How do you tell GDB this is a 64-bit RISC-V target?

   Answer:

   ```gdb
   set architecture riscv:rv64
   ```

4. How do you break at an exact address?

   Answer:

   ```gdb
   break *0x80000000
   ```

5. Why might you use a raw address breakpoint in ECE391-style debugging?

   Answer: early kernel code may run before output exists, and symbol information may be missing, stale, or loaded at the wrong address.
