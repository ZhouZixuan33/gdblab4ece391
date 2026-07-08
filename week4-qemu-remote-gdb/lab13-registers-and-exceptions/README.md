# Lab 13: RISC-V Registers and Exception-Style Failures

## Goal

Use GDB to inspect CPU state around controlled low-level RISC-V failures.

You will practice:

```gdb
info registers
x/i $pc
x/10i $pc
x/32gx $sp
disassemble
```

## Failure Scenarios

This lab has selectable scenarios:

```bash
make SCENARIO=good
make SCENARIO=bad-pointer
make SCENARIO=bad-jump
make SCENARIO=illegal-instruction
```

- `good`: confirms your QEMU/GDB setup.
- `bad-pointer`: sets `a0 = 0xdeadbeef` before a suspicious load.
- `bad-jump`: jumps through `t0` to an unexpected landing point.
- `illegal-instruction`: executes an intentionally illegal RISC-V instruction word.

These are exception-style labs. The point is to stop before or at the bad instruction and inspect machine state before guessing from source.

## Concept Warmup

When source-level explanation is not enough, ask:

```text
What exact instruction is pc pointing at?
What registers does that instruction use?
Does sp still look plausible?
```

Useful commands:

```gdb
info registers
x/i $pc
x/10i $pc
x/32gx $sp
```

If the target resets or traps too quickly, use:

```bash
make SCENARIO=illegal-instruction log
```

The log file is written under the scenario build directory, such as:

```text
build/illegal-instruction/qemu.log
```

## Guided Mode

Step 1: Prove the environment with the good scenario.

```bash
make SCENARIO=good run
```

Step 2: Build and start the bad pointer scenario under GDB.

```bash
make SCENARIO=bad-pointer debug
```

In another terminal:

```bash
gdb-multiarch build/bad-pointer/kernel.elf
```

```gdb
set architecture riscv:rv64
target remote :1234
```

Step 3: Stop before the suspicious load.

```gdb
break bad_pointer_fault_site
continue
```

Step 4: Inspect the instruction and registers.

```gdb
info registers pc sp ra a0 a1
x/i $pc
p/x $a0
x/32gx $sp
```

What to look for: `a0` should be `0xdeadbeef`, and the current instruction loads memory through `a0`.

Step 5: Debug the bad jump scenario.

Restart QEMU:

```bash
make SCENARIO=bad-jump debug
```

In GDB:

```gdb
break bad_jump_source
break unexpected_landing
continue
info registers pc t0
x/i $pc
continue
```

Step 6: Debug the illegal instruction scenario.

Restart QEMU:

```bash
make SCENARIO=illegal-instruction debug
```

In GDB:

```gdb
break illegal_instruction_site
continue
x/i $pc
info registers pc sp mcause mepc mtval
```

Step 7: Use a QEMU log when continuing loses the original clue.

```bash
make SCENARIO=illegal-instruction log
```

## Review Questions

1. What command shows all registers?

   Answer:

   ```gdb
   info registers
   ```

2. How do you show the current RISC-V instruction?

   Answer:

   ```gdb
   x/i $pc
   ```

3. In `bad-pointer`, which register holds the suspicious address?

   Answer: `a0`.

4. Which register replaces the x86 `eip` habit?

   Answer: `pc`.

5. Why inspect before continuing through the bad instruction?

   Answer: after a trap or reset-like failure, the original clue may be harder to recover.
