# Lab 10: Mixed C and Assembly Debugging

## Goal

Understand the 32-bit x86 calling convention used when C code calls assembly code, especially how caller-saved and callee-saved registers divide responsibility across a function call.

Use `disassemble`, `info registers`, `x/i $eip`, `stepi`, `nexti`, and `x/16xw $esp` to debug across a C/assembly boundary and recognize a callee-saved register violation.

By the end of this lab, students should understand that a calling convention is the low-level contract that explains:

- how arguments are passed
- where return values are placed
- who saves which registers
- how `call` and `ret` move control between functions
- how the stack must be kept balanced
- why hand-written assembly must follow the same rules as compiler-generated C code


## Setup Requirement / 环境要求

This lab builds a 32-bit x86 executable on a normal 64-bit Ubuntu machine. The Makefile uses:

```makefile
-m32
```

`-m32` tells GCC to generate 32-bit x86 code. On Ubuntu, this requires extra 32-bit development packages. Before starting Lab 09, install:

```bash
sudo apt update
sudo apt install gcc-multilib libc6-dev-i386
```

Why these packages matter:

- `gcc-multilib`: lets GCC build 32-bit programs on a 64-bit system.
- `libc6-dev-i386`: provides 32-bit C library headers and startup files. Without it, even `#include <stdio.h>` can fail.

If you see this error:

```text
fatal error: bits/libc-header-start.h: No such file or directory
```

it usually means the 32-bit C development headers are missing. Install the packages above, then run:

ECE391-style x86 work uses the 32-bit register view much more often:

```text
eax
ebx
ecx
edx
esp
ebp
eip
```

This is why Lab 09/Lab10 uses `-m32`. It lets you practice with the register names and stack behavior that match the 32-bit x86 mental model used later.

When you run:

```gdb
info registers
```

you should learn to find these first:

```text
eip: instruction pointer, where the CPU is executing
esp: stack pointer, where the current stack top is
ebp: frame pointer, a stable anchor for the current function frame
eax: common integer return-value register
```

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

### Calling Convention / 调用约定

A calling convention is an agreement between the caller and the callee about how a function call works at the machine-instruction level.

In C, a function call looks simple:

```c
result = helper(1, 2, 3);
```

But the CPU does not understand "function call" as a high-level C idea. At the assembly level, the program has to answer concrete questions:

```text
Where do the arguments go?
Where does the return value go?
Which instruction jumps into the function?
How does the function jump back?
Who cleans up the stack?
Which registers may change?
Which registers must survive?
```

The calling convention answers these questions so separately compiled code can still work together. This matters a lot when C code calls hand-written assembly, because the C compiler will follow the convention automatically, but your assembly code must follow it manually.

For the simple 32-bit x86 style used in these labs, the beginner model is:

```text
arguments are passed on the stack
call pushes the return address
ret jumps back through that return address
eax usually holds the integer return value
eax/ecx/edx are caller-saved
ebx/esi/edi/ebp are callee-saved
esp is the live stack pointer and must stay balanced
```

Calling convention usually includes several related rules:

```text
Argument passing:
    Are arguments passed on the stack or in registers?

Return value:
    Which register carries the return value back to the caller?

Register preservation:
    Which registers may the callee freely change?
    Which registers must the callee restore before returning?

Stack discipline:
    Who pushes arguments?
    Who removes arguments?
    Where are local variables stored?
    What must be true about esp before ret?

Control flow:
    What does call do?
    What does ret do?
    Where is the return address stored?

Frame layout:
    Is ebp used as a stable frame pointer?
    Where are arguments and local variables relative to ebp?
```

This lab focuses mostly on register preservation, because `broken_helper` violates the callee-saved rule for `ebx`.

This lab uses a mixed-debugging loop:

```text
stop before the assembly call -> record registers -> step into assembly -> inspect instructions -> compare registers after return -> identify the calling convention violation
```

### Register Responsibility / 寄存器责任

In ECE391-style 32-bit x86 debugging, the question is not "which registers can I use freely?" The better question is:

```text
Who is responsible for protecting this register across a function call?
```

Calling convention divides register responsibility so caller and callee do not have to guess what the other side will preserve.

Practical memory table:

```text
eax     return value, caller-saved
ecx     counter/temp, caller-saved
edx     data/temp, caller-saved

ebx     callee-saved
esi     callee-saved
edi     callee-saved
ebp     frame pointer, callee-saved, do not casually clobber

esp     stack pointer, must stay balanced
eip     instruction pointer, changed by call/ret/jmp/int/iret
eflags  condition/control flags
```

Meaning:

```text
eax/ecx/edx can be used for temporary values, but the caller must know a call may change them.
ebx/esi/edi can be used, but the callee must restore them if it changes them.
ebp is callee-saved too, but it often has a special job as the frame pointer.
esp is the stack top and must remain balanced.
eip is where the CPU is executing; control-flow instructions change it.
eflags is CPU state; many conditional jumps depend on it.
```

This is why a function may freely return a value in `eax`, but it may not freely destroy `ebx` or `ebp`.

### Caller-Saved Register / 调用者保存寄存器

Caller means the function that makes a function call.

Caller-saved does not mean "the CPU saves it for the caller." It means the caller is responsible for saving the value if the caller still needs it after the call.

On 32-bit x86, the most important caller-saved registers for this lab are:

```text
eax
ecx
edx
```

These registers are allowed to change across a function call. If a callee changes `eax`, `ecx`, or `edx`, that is usually not a callee bug.

Example:

```asm
movl $1234, %eax
call some_function
```

After `call some_function`, the caller should not assume `eax` is still `1234`. `some_function` may have used `eax` as a temporary register, and `eax` is also commonly used for the return value.

If the caller really needs the old `eax` value, the caller must protect it:

```asm
pushl %eax
call some_function
popl %eax
```

Beginner rule:

```text
If I am the caller and I need eax/ecx/edx after a call,
I must save them myself before the call.
```


### Callee-Saved Register / 被调用者保存寄存器

Callee means the function being called.

A callee-saved register must have the same value when a function returns as it had when the function was called.

On 32-bit x86, the most important callee-saved registers for this lab are:

```text
ebx
esi
edi
ebp
```

This does not mean the callee can never use these registers. It means the callee must restore the original value before returning if it changes one of them.

A correct callee that wants to use `ebx` usually does something like this:

```asm
pushl %ebx
movl  $0x0badc0de, %ebx
/* use ebx */
popl  %ebx
ret
```

The `pushl %ebx` saves the caller's old `ebx` value. The `popl %ebx` restores it before returning. From the caller's point of view, `ebx` survived the function call.

For this lab:

```text
ebx should be preserved by broken_helper
eax may be used for the return value
esp should still point to a valid stack
eip shows the current instruction
```

Beginner rule:

```text
If I am the callee and I change ebx/esi/edi/ebp,
I must put them back before returning.
```

The full picture is:

```text
caller-saved: caller protects the value if the caller still needs it
callee-saved: callee protects the value if the callee changes it
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
