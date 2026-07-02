# Lab 10: Mixed C and Assembly Debugging

## Goal

Use `disassemble`, `info registers`, `x/i $eip`, `stepi`, `nexti`, and `x/16xw $esp` to debug across a C/assembly boundary and recognize a callee-saved register violation.

## Failure Scenario

Run the program:

```bash
make run
```

Expected output:

```text
Mixed C/assembly register preservation check
check_preserves_ebx returned: 1
FAIL: broken_helper changed ebx without restoring it.
Expected callee-saved registers to survive a function call.
```

The C program calls `check_preserves_ebx()`, an assembly checker. The checker sets `ebx` to a sentinel value, calls `broken_helper`, then checks whether `ebx` survived the call.

`broken_helper` is intentionally wrong:

```asm
broken_helper:
    movl $0x0badc0de, %ebx
    movl $7, %eax
    ret
```

On 32-bit x86, `ebx` is callee-saved. A function that changes it must restore it before returning.

If the build fails with a `-m32` or missing header/library error, install 32-bit support:

```bash
sudo apt install gcc-multilib libc6-dev-i386
```

## Where This Shows Up / Common Scenarios

This pattern appears when execution crosses between C and assembly:

- context switch helpers
- interrupt or syscall entry stubs
- hand-written register save/restore code
- boot or kernel startup helpers
- functions that appear to corrupt unrelated C state after returning

In ECE391-style code, a register preservation bug may look like a random C variable changed. The real bug can be in the assembly boundary.

## Concept Warmup

This lab uses a mixed-debugging loop:

```text
stop before the assembly call -> record registers -> step into assembly -> inspect instructions -> compare registers after return -> identify the calling convention violation
```

### Callee-Saved Register / 被调用者保存寄存器

A callee-saved register must have the same value when a function returns as it had when the function was called.

For this lab:

```text
ebx should be preserved by broken_helper
eax may be used for the return value
esp should still point to a valid stack
eip shows the current instruction
```

### Instruction-Level Stepping / 指令级单步

Useful commands:

```gdb
disassemble broken_helper
x/i $eip
stepi
nexti
info registers
```

`stepi` executes one machine instruction and enters calls.

`nexti` executes one machine instruction but steps over calls.

## Guided Mode

Step 1: Build and run.

```bash
make
make run
```

What to look for / 看什么: the checker returns `1`, meaning `ebx` changed across a function call that should have preserved it.

Step 2: Start GDB and set breakpoints.

```bash
gdb ./build/lab10
```

```gdb
break main
break check_preserves_ebx
break broken_helper
run
```

Step 3: Continue to the assembly checker.

```gdb
continue
info registers
disassemble check_preserves_ebx
```

What to look for / 看什么: the checker saves the original `ebx`, writes sentinel `0x39139139` into `ebx`, then calls `broken_helper`.

Step 4: Stop in `broken_helper`.

```gdb
continue
info registers
x/i $eip
disassemble broken_helper
```

What to look for / 看什么: `broken_helper` is about to execute instructions that write to `ebx`.

Step 5: Step the bad instruction.

```gdb
stepi
info registers ebx eax eip esp ebp
```

What to look for / 看什么: `ebx` becomes `0x0badc0de`.

Step 6: Return to the checker.

```gdb
finish
info registers ebx eax eip esp ebp
```

What to look for / 看什么: the checker observes that `ebx` no longer equals the sentinel.

Step 7: Inspect stack context.

```gdb
x/16xw $esp
bt
```

Why this helps / 为什么有用: mixed C/assembly bugs can damage the stack too. This lab does not intentionally corrupt `esp`, but you should practice checking it.

Step 8: Explain the fix.

The broken helper should save and restore `ebx`:

```asm
broken_helper:
    pushl %ebx
    movl $0x0badc0de, %ebx
    movl $7, %eax
    popl %ebx
    ret
```

## Hint Mode

1. Run the program and note the checker return value.
2. Break on `check_preserves_ebx` and `broken_helper`.
3. Disassemble both assembly functions.
4. Record `ebx` before and after stepping through `broken_helper`.
5. Use `x/i $eip` to identify the current instruction.
6. Explain which register preservation rule was violated.

## Review Questions

1. What command disassembles the assembly helper?

   Answer:

   ```gdb
   disassemble broken_helper
   ```

2. What command shows the instruction at the current instruction pointer?

   Answer:

   ```gdb
   x/i $eip
   ```

3. What command steps one machine instruction?

   Answer:

   ```gdb
   stepi
   ```

4. Which register is intentionally clobbered in this lab?

   Answer: `ebx`.

5. Why is clobbering `ebx` a bug here?

   Answer: `ebx` is callee-saved in this 32-bit calling convention, so a function that changes it must restore it before returning.

6. What is the conceptual fix?

   Answer: save `ebx` before modifying it and restore it before `ret`, for example with `pushl %ebx` and `popl %ebx`.
