# Lab 10: Mixed C and RISC-V Assembly Inspection

## Goal

Understand the RV32 calling convention used when C code calls hand-written RISC-V assembly.

Week 3 stays at the static build/disassembly layer. You will compile C and `.S` files into RV32 object files, inspect symbols and disassembly, and identify a callee-saved register violation without using QEMU yet.

By the end of this lab, students should understand:

- how C can call a function implemented in `.S`
- where RV32 return values appear
- which registers are caller-saved
- which registers are callee-saved
- why assembly code must follow the same calling convention as compiler-generated C code

Week 4 will use QEMU and GDB remote debugging to observe live register values while a target is running.

## Setup Requirement / 环境要求

This lab uses the RISC-V cross toolchain:

```bash
sudo apt install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf make
```

The Makefile uses:

```text
riscv64-unknown-elf-gcc
-march=rv32im
-mabi=ilp32
```

This lab produces object files and disassembly artifacts:

```text
build/main.o
build/asm_helpers.o
build/lab10.combined.o
build/main.s
build/asm_helpers.dump
build/lab10.combined.dump
```

It does not produce a runnable QEMU target.

## Failure Scenario

Build the lab:

```bash
make
```

The C file declares:

```c
int check_preserves_s1(void);
```

`check_preserves_s1` is implemented in `asm_helpers.S`. It sets `s1` to a sentinel value, calls `broken_helper`, then checks whether `s1` survived the call.

`broken_helper` is intentionally wrong:

```asm
broken_helper:
    li s1, 0x0badc0de
    li a0, 7
    ret
```

On RV32, `s1` is callee-saved. A function that changes it must restore it before returning.

The static bug to find:

```text
broken_helper writes s1
broken_helper does not save old s1
broken_helper does not restore old s1 before ret
```

## Concept Warmup

### Calling Convention / 调用约定

A calling convention is an agreement between caller and callee about how a function call works at the machine-instruction level.

For RV32, the practical model is:

```text
a0-a7       argument registers and return-value registers
t0-t6       temporary registers, caller-saved
s0-s11      saved registers, callee-saved
ra          return address register
sp          stack pointer
pc          current instruction address
```

This lab focuses on register preservation. `broken_helper` violates the callee-saved rule for `s1`.

### What Is a `.S` File? / `.S` 文件是什么

Lab 10 has two source files:

```text
main.c         C source file
asm_helpers.S RISC-V assembly helper functions
```

The uppercase `.S` extension means assembly source code that is processed by the C preprocessor before it is assembled.

Difference:

```text
.s   ordinary assembly source, usually sent directly to the assembler
.S   preprocessed assembly source, then assembled
```

This is useful in kernel-style code because assembly files often need constants, macros, or offsets shared with C code.

Build flow:

```text
main.c         -> build/main.o
asm_helpers.S -> build/asm_helpers.o
main.o + asm_helpers.o -> build/lab10.combined.o
```

The combined object is not a runnable program. It is a convenient artifact for symbol and disassembly inspection.

### Compile and Link / 编译和链接

Compile C:

```bash
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -g -O0 -fno-omit-frame-pointer -Wall -Wextra -std=c11 -c main.c -o build/main.o
```

Assemble `.S`:

```bash
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -g -O0 -fno-omit-frame-pointer -c asm_helpers.S -o build/asm_helpers.o
```

Combine object files:

```bash
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -r build/main.o build/asm_helpers.o -o build/lab10.combined.o
```

Why combine them?

```text
main.o refers to check_preserves_s1
asm_helpers.o defines check_preserves_s1
the combined object lets you inspect both sides together
```

### Register Responsibility / 寄存器责任

The question is not "which registers can I use freely?" The better question is:

```text
Who is responsible for protecting this register across a function call?
```

Practical RV32 memory table:

```text
a0-a7   arguments and return values, caller-saved
t0-t6   temporary registers, caller-saved
s0-s11  saved registers, callee-saved
ra      return address, caller-saved in the ABI sense
sp      stack pointer, must stay balanced
pc      current instruction address
```

Meaning:

```text
a-registers can pass arguments and return values, but calls may change them.
t-registers are temporary; callers should not expect them to survive calls.
s-registers may be used by a callee, but the callee must restore them.
sp must point to a valid stack and be restored before returning.
ra tells ret where to go back.
```

### Caller-Saved Register / 调用者保存寄存器

Caller means the function that makes a function call.

Caller-saved means:

```text
If the caller still needs the value after a call,
the caller must save it before the call.
```

Important RV32 caller-saved registers:

```text
a0-a7
t0-t6
ra
```

After a call, the caller should not assume `a0` still contains the old argument value. `a0` may contain the callee's return value.

### Callee-Saved Register / 被调用者保存寄存器

Callee means the function being called.

Callee-saved means:

```text
If the callee changes the register,
the callee must restore the original value before returning.
```

Important RV32 callee-saved registers:

```text
s0-s11
```

A correct callee that wants to use `s1` usually does something like this:

```asm
addi sp, sp, -16
sw   s1, 12(sp)

li   s1, 0x0badc0de
# use s1

lw   s1, 12(sp)
addi sp, sp, 16
ret
```

Lab 10 is built around this exact rule. `broken_helper` changes `s1` but does not restore it.

## Guided Mode

Step 1: Build artifacts.

```bash
make
```

Step 2: Inspect symbols.

```bash
riscv64-unknown-elf-nm build/main.o build/asm_helpers.o build/lab10.combined.o
```

What to look for / 看什么:

```text
main.o has an undefined reference to check_preserves_s1 before combining
asm_helpers.o defines check_preserves_s1
asm_helpers.o defines broken_helper
```

Step 3: Inspect assembly helper disassembly.

```bash
less build/asm_helpers.dump
```

Or:

```bash
riscv64-unknown-elf-objdump -dr build/asm_helpers.o
```

What to look for / 看什么:

```text
check_preserves_s1 saves s1 before using it
broken_helper writes s1
broken_helper returns without restoring s1
```

Step 4: Inspect the combined object.

```bash
less build/lab10.combined.dump
```

What to look for / 看什么: C-side symbol references and assembly-side symbol definitions are now in one object artifact.

Step 5: Explain the bug.

Expected explanation:

```text
s1 is callee-saved in RV32.
broken_helper is a callee.
broken_helper modifies s1.
broken_helper does not restore the original s1 before ret.
Therefore broken_helper violates the calling convention.
```

## Fix Idea

The broken helper should save and restore `s1`:

```asm
broken_helper:
    addi sp, sp, -16
    sw   s1, 12(sp)
    li   s1, 0x0badc0de
    li   a0, 7
    lw   s1, 12(sp)
    addi sp, sp, 16
    ret
```

This lab intentionally leaves the bug in place so students can practice finding it.

## Review Questions

1. Which RV32 register carries the integer return value?

   Answer: `a0`.

2. Which RV32 register carries the return address?

   Answer: `ra`.

3. Is `s1` caller-saved or callee-saved?

   Answer: callee-saved.

4. Why is clobbering `s1` a bug here?

   Answer: `s1` is callee-saved, so a function that changes it must restore it before returning.

5. How should `broken_helper` fix the bug?

   Answer: save `s1` before modifying it and restore `s1` before `ret`.
