# Lab 14: RISC-V Mini-Kernel Debug Triage

## Goal

Practice choosing the first useful QEMU/GDB action from the symptom.

This lab combines the Week 4 tools:

- `target remote :1234`
- `break function_name`
- `break *addr`
- `info registers`
- `x/i $pc`
- `x/32gx $sp`
- QEMU `-d int,cpu_reset`

## Failure Scenarios

Select one scenario:

```bash
make SCENARIO=good
make SCENARIO=hang
make SCENARIO=wrong-entry
make SCENARIO=reset
```

- `good`: reaches `expected_checkpoint`.
- `hang`: reaches `early_boot_hang` and spins forever.
- `wrong-entry`: jumps to `wrong_entry_target` instead of the expected checkpoint.
- `reset`: sets `mtvec = 0` and executes `ecall`, creating trap/reset-like behavior.

## Concept Warmup

Before reading source, classify the symptom:

```text
No output yet: connect GDB, inspect pc, break at entry.
Some output then hang: interrupt or break near the last checkpoint.
Wrong checkpoint: compare pc with symbol addresses.
Reset or black screen: enable QEMU int/cpu_reset logging.
```

The RISC-V register habits are:

```text
pc  current instruction address
sp  stack pointer
ra  return address
a0  first argument / common return-value register
```

## Guided Mode

Step 1: Run the good scenario.

```bash
make SCENARIO=good run
```

Step 2: Debug the hang scenario.

```bash
make SCENARIO=hang debug
```

In another terminal:

```bash
gdb-multiarch build/hang/kernel.elf
```

```gdb
set architecture riscv:rv64
target remote :1234
```

Step 3: Stop at the known hang label.

```gdb
break early_boot_hang
continue
info registers pc sp ra
x/i $pc
```

What to look for: execution reaches `early_boot_hang`, then stays in a short spin loop.

Step 4: Inspect the stack.

```gdb
x/32gx $sp
bt
```

In a tiny freestanding target, `bt` may be limited. `sp` and raw stack words are still useful.

Step 5: Debug the wrong-entry scenario.

Restart QEMU:

```bash
make SCENARIO=wrong-entry debug
```

In GDB:

```gdb
break kernel_entry
break expected_checkpoint
break wrong_entry_target
continue
continue
info registers pc sp
x/i $pc
```

What to look for: execution reaches `wrong_entry_target`, not `expected_checkpoint`.

Step 6: Compare symbols and addresses.

```bash
make SCENARIO=wrong-entry symbols
```

In GDB:

```gdb
info files
info functions
```

Step 7: Investigate the reset scenario with logs.

```bash
make SCENARIO=reset log
```

What to look for: `build/reset/qemu.log` should contain trap or CPU reset clues.

Step 8: Debug the reset scenario before it traps.

```bash
make SCENARIO=reset debug
```

In GDB:

```gdb
break reset_trigger
continue
x/i $pc
info registers pc sp mtvec mcause mepc
```

What to look for: stop before the lab sets `mtvec` to zero and executes `ecall`.

## Review Questions

1. What is the first GDB command after QEMU starts with `-s -S`?

   Answer:

   ```gdb
   target remote :1234
   ```

2. What do you inspect first during a hang?

   Answer: `pc`, the current instruction, and nearby disassembly.

3. How do you check whether execution reached the expected checkpoint?

   Answer: set breakpoints on both the expected checkpoint and suspicious alternate target.

4. What command creates a QEMU exception/reset log in this lab?

   Answer:

   ```bash
   make SCENARIO=reset log
   ```

5. Why might `bt` be less helpful in a tiny kernel target than in a Linux process?

   Answer: there is no normal process runtime, and freestanding assembly may not provide enough frame information. Registers, stack memory, symbols, and disassembly become more important.
