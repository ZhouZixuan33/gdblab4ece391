# Lab 09: 32-bit x86 Calling Convention

## Goal

Learn why this lab is compiled with `-m32`, then use `disassemble`, `info registers`, `x/16xw $esp`, `si`, `ni`, and `finish` to connect a C function call to the 32-bit x86 stack, registers, arguments, and return value.

The practical ECE391 goal is to build the machine-level debugging habits needed for 32-bit x86 code: reading `esp`, `ebp`, `eip`, and `eax`; understanding how C function calls pass arguments and return values; and preparing for syscall, interrupt, QEMU, and C/assembly debugging later in the course.

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

```bash
make clean
make
make run
```

## Failure Scenario

Run the program:

```bash
make run
```

Expected output shape:

```text
Request args: arg0=1 arg1=2 arg2=3
Expected encode_triple(arg0, arg1, arg2): 321
Actual dispatched encoding: 231
Expected the dispatcher to preserve argument order.
```

The bug is in `dispatch_request`. It calls:

```c
encode_triple(request->arg0, request->arg2, request->arg1)
```

instead of:

```c
encode_triple(request->arg0, request->arg1, request->arg2)
```

The C bug is simple on purpose. The real practice is to stop inside `encode_triple` and see how 32-bit x86 passes arguments and returns values.

## Why ECE391 Needs This / 为什么 ECE391 要学这个

This lab is not mainly about memorizing the `-m32` compiler flag. The point is to enter the machine-level debugging world that ECE391-style work uses all the time.

In earlier labs, you mostly debugged ordinary user-space C programs. That workflow starts from source code:

```text
break function
run
next
print variable
bt
info locals
info args
```

Those commands are still useful. But in operating-system-style work, source-level debugging is not always enough. You may need to answer questions like:

```text
Where is the CPU executing right now?
What address is the instruction pointer holding?
Where does the stack pointer point?
What words are sitting on the stack?
Which register contains the return value?
Did the caller pass arguments in the right order?
Did an assembly helper preserve the registers it promised to preserve?
Did a bad stack pointer make the backtrace unreliable?
```

Those are not abstract questions. They show up when debugging low-level code, assembly boundaries, exception paths, syscall paths, and QEMU targets.

### ECE391-Style Debugging Uses 32-bit x86 Names

Modern Linux machines are often 64-bit, so ordinary programs commonly use register names like:

```text
rax
rbx
rcx
rdx
rsp
rbp
rip
rdi
rsi
```

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

This is why Lab 09 uses `-m32`. It lets you practice with the register names and stack behavior that match the 32-bit x86 mental model used later.

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

The goal is not to understand every register immediately. The goal is to stop freezing when GDB shows machine state.

### Source-Level Debugging Can Lie by Omission

Source-level GDB output is friendly:

```gdb
info args
info locals
list
```

But source-level output hides details that matter in low-level bugs.

For example, if a function receives the wrong argument, source view can tell you:

```text
second = 3
third = 2
```

but machine view helps you answer:

```text
How did those values arrive?
Were they pushed on the stack in this order?
What did the caller do before call?
What return address did the call instruction place on the stack?
Where will ret jump when the callee finishes?
```

That matters because kernel-style failures often happen at boundaries:

```text
C calls assembly
assembly calls C
interrupt/exception code enters a handler
syscall entry code transfers control
context switch code saves and restores registers
QEMU stops at an instruction without a friendly C variable nearby
```

At those boundaries, the source code may not tell the whole story. You need registers, stack words, and disassembly.

### Calling Convention Is the Contract Between Caller and Callee

A calling convention is a contract. It answers:

```text
How does the caller pass arguments?
Where does the callee put the return value?
Which registers may the callee freely change?
Which registers must the callee restore before returning?
Who cleans up the stack?
```

For the simple 32-bit C calls in this lab, the important beginner model is:

```text
caller pushes arguments on the stack
the call instruction pushes a return address
callee sets up a stack frame
callee returns an integer value in eax
ret jumps back through the saved return address
```

That is why these commands matter:

```gdb
disassemble
info registers
x/16xw $esp
x/8xw $ebp
finish
p/d $eax
```

They let you connect the C-level story:

```c
encode_triple(first, second, third)
```

to the machine-level story:

```text
arguments are stack words
eip points at the current instruction
esp/ebp describe the current stack frame
eax holds the return value after the function finishes
```


### What You Should Be Able to Say After This Lab

After this lab, you should be able to explain a simple function call from two views.

C view:

```text
main calls dispatch_request
dispatch_request calls encode_triple
encode_triple receives first, second, third
the wrong result comes from swapped arguments
```

Machine view:

```text
the caller passes values according to the 32-bit calling convention
the callee runs with eip pointing at its instructions
esp and ebp describe the active stack frame
stack words contain call-related data such as arguments and return address
eax contains the integer return value
```

That bridge from C view to machine view is the real reason ECE391 needs this topic.

## Concept Warmup

This lab uses a C-to-machine debugging loop:

```text
stop in callee -> inspect C args -> inspect esp/ebp/eip/eax -> examine stack words -> step instructions -> connect return value to eax
```

### Calling Convention / 调用约定

A calling convention is the rule for how a caller passes arguments, how a callee returns values, and which registers must be preserved.

For simple 32-bit x86 C calls in this lab:

```text
arguments: pushed by the caller on the stack
return value: placed in eax
esp: moving stack pointer, points at the current top of the stack
ebp: stable frame pointer, anchors the current function's stack frame at -O0
eip: instruction pointer
```

### Stack View / 栈视角

When stopped inside `encode_triple`, inspect the stack:

```gdb
x/16xw $esp
x/8xw $ebp
```

Typical frame shape:

```text
Higher addresses
arg3
arg2
arg1
return address
saved ebp        <- ebp often points here
locals / temps
scratch space    <- esp often points lower in the frame
Lower addresses
```

### Concrete Stack Example / 具体栈例子

Here is an example stop inside `encode_triple`. Your exact addresses can differ, but the relationship between `$esp`, `$ebp`, arguments, local stack space, and `$eax` is the important part.

```gdb
(gdb) info args
first = 1
second = 2
third = 3
(gdb) info registers
eax            0x8                 8
ecx            0x3                 3
edx            0x3                 3
esp            0xffffd280          0xffffd280
ebp            0xffffd2a8          0xffffd2a8
eip            0x80491dc           0x80491dc <encode_triple+86>
(gdb) n
21      }
(gdb) info registers
eax            0x141               321
ecx            0x3                 3
edx            0x3                 3
esp            0xffffd280          0xffffd280
ebp            0xffffd2a8          0xffffd2a8
eip            0x80491df           0x80491df <encode_triple+89>
(gdb) x/16xw $esp
0xffffd280:     0x00000000      0x00000001      0x00000141      0x00000003
0xffffd290:     0x00000005      0x00000141      0x00000008      0xdd796b00
0xffffd2a0:     0xf7ffcfe8      0x00000018      0xffffd2e8      0x0804925e
0xffffd2b0:     0x00000001      0x00000002      0x00000003      0x00000000
(gdb) x/16xw $ebp
0xffffd2a8:     0xffffd2e8      0x0804925e      0x00000001      0x00000002
0xffffd2b8:     0x00000003      0x00000000      0x00000000      0x00000000
0xffffd2c8:     0x00000000      0x00000000      0x00000001      0x00000002
0xffffd2d8:     0x00000003      0xdd796b00      0x00000000      0xffffd300
```

GDB's `info args` output is the C view:

```text
first = 1
second = 2
third = 3
```

The first `info registers` output stops before the function has placed the final return value in `$eax`:

```text
eip = 0x80491dc <encode_triple+86>
eax = 0x8 = 8
```

After one `next`, GDB reaches the closing brace and `$eax` holds the return value:

```text
eip = 0x80491df <encode_triple+89>
eax = 0x141 = 321
```

This is a useful reminder: `$eax` is the return-value register, but you should inspect it after the function has actually computed or returned the value. If you stop too early, `$eax` may still contain a temporary value.

The `x/16xw $ebp` output is the frame-anchor view. Each word is 4 bytes:

```text
address       value         meaning
0xffffd2a8    0xffffd2e8    saved old ebp
0xffffd2ac    0x0804925e    return address
0xffffd2b0    0x00000001    first argument
0xffffd2b4    0x00000002    second argument
0xffffd2b8    0x00000003    third argument
```

This is the classic beginner 32-bit argument pattern from `$ebp`:

```text
ebp + 0   saved old ebp
ebp + 4   return address
ebp + 8   first argument
ebp + 12  second argument
ebp + 16  third argument
```

The `x/16xw $esp` output is the current-stack-top view. In this example, `$esp` is lower than `$ebp`, so it points into the local/temporary part of the current function's stack frame:

```text
address       value         likely meaning in this lab
0xffffd280    0x00000000    local/temp stack area
0xffffd284    0x00000001    local/temp stack area
0xffffd288    0x00000141    local/temp stack area or encoded value
0xffffd28c    0x00000003    local/temp stack area
0xffffd290    0x00000005    local/temp stack area
0xffffd294    0x00000141    local/temp stack area or encoded value
0xffffd298    0x00000008    local/temp stack area
0xffffd29c    0xdd796b00    local/temp stack area or old stack data
0xffffd2a0    0xf7ffcfe8    local/temp stack area or old stack data
0xffffd2a4    0x00000018    local/temp stack area
```

Do not try to assign a perfect C variable name to every word near `$esp`. Some words may be local variables, compiler temporaries, alignment padding, or old stack data. The teaching point is stable:

```text
ebp + positive offsets: caller data, return address, arguments
ebp + 0:                saved old ebp
ebp - negative offsets: callee locals and temporary stack storage
esp:                    current lowest/top point of this active stack frame
```

What to remember:

- `$ebp` is the stable anchor for the current function frame.
- `$esp` is the moving top of the stack. It moves lower when the function reserves local stack space.
- In this lab, `$esp` should usually be lower than `$ebp` because `encode_triple` has local variables.
- Use `$ebp` when you want a stable way to find arguments and the return address.
- Use `$esp` when you want to see the current active stack top, local stack storage, temporary values, or whether the stack still looks plausible.
- The return address, `0x0804925e`, is where `ret` will jump after `encode_triple` finishes.
- The arguments `1`, `2`, and `3` appear at `ebp + 8`, `ebp + 12`, and `ebp + 16`.
- `$eax` may hold temporary values while the function is still executing.
- `$eax = 321` at the closing brace means the final integer result is now in the normal return-value register.

You can verify the return address with:

```gdb
x/i 0x0804925e
```

It should show an instruction in the caller after the `call encode_triple` instruction.


### Why Use ebp? / 为什么要用 ebp

For beginner debugging, `ebp` is useful because it often gives you a stable map of the current function call:

```text
ebp + 0   saved old ebp
ebp + 4   return address
ebp + 8   first argument
ebp + 12  second argument
ebp + 16  third argument

ebp - 4   local variable or temporary storage
ebp - 8   local variable or temporary storage
```

`esp` moves as the function uses stack space:

```text
push      changes esp
pop       changes esp
call      changes esp because it pushes the return address
ret       changes esp because it pops the return address
local variables may make esp move lower
```

So if you use only `esp`, the meaning of an address can shift as the function runs. `ebp` stays stable after the function prologue, so it is much easier for a beginner to find arguments, return address, and local variables.


```text
use ebp to understand the frame
use esp to understand the live stack top
do not casually overwrite either one in hand-written assembly
```

## Guided Mode

Step 1: Build and run.

```bash
make
make run
```

What to look for / 看什么: `expected` is `321`, but the dispatched value is `231`.

Step 2: Start GDB and stop inside the callee.

```bash
gdb ./build/lab09
```

```gdb
break encode_triple
run
```

What to look for / 看什么: the first stop may be from the expected calculation in `main`. Use `continue` to reach the dispatcher call if needed.

Step 3: Inspect C-level arguments.

```gdb
info args
bt
```

What to look for / 看什么:

```text
first = 1
second = 3
third = 2
```

That explains `231`: the second and third arguments were swapped before entering the function.

Step 4: Inspect registers.

```gdb
info registers
p/x $esp
p/x $ebp
p/x $eip
```

Meaning / 是什么: show the CPU state. On 32-bit x86, `esp`, `ebp`, `eip`, and `eax` are the first registers to orient yourself.

Step 5: Inspect stack words.

```gdb
x/16xw $esp
x/8xw $ebp
```

What to look for / 看什么:

- Near `$ebp`, you should see saved frame data, the return address, and the function arguments as 4-byte words.
- Near `$esp`, you should see the current active stack top. In this lab, that includes local stack storage from `encode_triple`.
- `$esp` should usually be lower than `$ebp` in this lab because the function reserves space for locals.

Step 6: Disassemble and step.

```gdb
disassemble encode_triple
si
ni
```

Meaning / 是什么:

- `disassemble` shows machine instructions for the current function.
- `si` steps one source line or one instruction when at instruction level.
- `ni` steps over calls at instruction level.

Step 7: Finish the function and inspect the return value.

```gdb
finish
p/d $eax
p/x $eax
```

What to look for / 看什么: `eax` holds the integer return value.

Step 8: Find the caller bug.

```gdb
up
list dispatch_request
```

What to look for / 看什么: `dispatch_request` passes `arg2` before `arg1`.

## Hint Mode

1. Run the program and identify the wrong value.
2. Break on `encode_triple`.
3. Use `continue` until the stop has `second = 3` and `third = 2`.
4. Print `info registers`.
5. Examine words around `$esp` and `$ebp`.
6. Disassemble the function.
7. Use `finish` and inspect `$eax`.
8. Move to the caller and inspect the call expression.

## Review Questions

1. Which register is the 32-bit x86 instruction pointer?

   Answer: `eip`.

2. Which register usually holds an integer return value in this lab?

   Answer: `eax`.

3. What command examines 16 hexadecimal words starting at the stack pointer?

   Answer:

   ```gdb
   x/16xw $esp
   ```

4. What command shows the current function's arguments?

   Answer:

   ```gdb
   info args
   ```

5. What is the actual C bug?

   Answer: `dispatch_request` swaps `arg1` and `arg2` when calling `encode_triple`.

6. Why compile this lab with `-fno-omit-frame-pointer`?

   Answer: it keeps an easy-to-read frame pointer so `$ebp` is useful while learning stack layout.
