# Lab 09: RISC-V 32 Registers, Calling Convention, and ELF

## Goal

Learn how ordinary C code becomes RV32 object code, then use build artifacts to understand:

- RISC-V registers
- RISC-V calling convention
- what an ELF file is
- ELF sections
- ELF symbols
- what an entry point means

This lab is concept-driven. It does not require QEMU and it does not require a runnable program. Week 3 stays at the build/disassembly layer; Week 4 starts QEMU and GDB remote debugging.

## Lab Design Plan / 本实验设计计划

The learning path is:

```text
1. C source code
   Understand that main.c is ordinary C, not RISC-V-specific by itself.

2. Compiler target
   Use riscv64-unknown-elf-gcc with -march=rv32im and -mabi=ilp32
   to generate RV32 code.

3. Generated assembly
   Inspect build/main.s to see readable RV32 assembly emitted by the compiler.

4. ELF object file
   Inspect build/main.o as an ELF relocatable object file.

5. Sections
   Learn where instructions, data, and debug information live inside ELF.

6. Symbols
   Learn how names such as main, dispatch_request, and encode_triple appear
   in object files.

7. Calling convention
   Connect C function arguments to a0/a1/a2 and return values to a0.

8. Bridge to Lab 10
   Prepare for C calling hand-written .S assembly, where the calling
   convention becomes the assembly programmer's responsibility.
```

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

Meaning:

```text
riscv64-unknown-elf-gcc   cross compiler that runs on your host machine
                           but generates RISC-V ELF output

-march=rv32im             generate 32-bit RISC-V integer instructions
                           with multiply/divide support

-mabi=ilp32               use the 32-bit RISC-V ABI:
                           int, long, and pointer are 32 bits
```

Although the compiler name contains `riscv64`, it can generate RV32 code when the target flags say:

```text
-march=rv32im -mabi=ilp32
```

This lab produces object files and assembly/disassembly artifacts for inspection:

```text
build/main.o      RV32 ELF relocatable object file
build/main.s      compiler-generated RV32 assembly listing
build/main.dump   objdump disassembly with relocations
```

## Inspection Target / 观察目标

Build the lab:

```bash
make
```

The C code contains this call:

```c
encode_triple(request->arg0, request->arg2, request->arg1)
```

The point is not to run the program. The point is to inspect the generated RV32 code and answer:

```text
Which value goes into a0?
Which value goes into a1?
Which value goes into a2?
Where does the return value come back?
Which symbols exist in the object file?
Which sections exist in the object file?
Does this object file have a meaningful entry point yet?
```

## Concept Warmup

### C Compiler Generates RV32 Code / C 编译器生成 RV32 代码

`main.c` is ordinary C source code. It is not RISC-V-specific by itself.

The target architecture is chosen by the compiler and flags:

```text
riscv64-unknown-elf-gcc   use a RISC-V cross compiler
-march=rv32im             generate RV32 instructions
-mabi=ilp32               use the 32-bit RISC-V ABI
```

Generate readable assembly:

```bash
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -S main.c -o build/main.s
```

Path:

```text
main.c -> build/main.s
```

Generate an object file:

```bash
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -c main.c -o build/main.o
```

Path:

```text
main.c -> build/main.o
```

Disassemble the object file:

```bash
riscv64-unknown-elf-objdump -dr build/main.o
```

Full inspection path:

```text
main.c
  -> build/main.s      compiler-generated RV32 assembly text
  -> build/main.o      RV32 ELF relocatable object file
  -> build/main.dump   objdump view of the object file's machine code
```

Why this matters:

```text
The C source says encode_triple(first, second, third).
The RV32 calling convention decides which registers carry those values.
The compiler generates instructions that place values into a0, a1, and a2.
```

### RISC-V Registers / RISC-V 寄存器

Practical beginner table:

```text
zero / x0     always 0
ra   / x1     return address
sp   / x2     stack pointer
gp   / x3     global pointer
tp   / x4     thread pointer
t0-t6         temporary registers, caller-saved
s0/fp         saved register / frame pointer, callee-saved
s1-s11        saved registers, callee-saved
a0-a7         argument registers and return-value registers
pc            program counter, current instruction address
```

For this lab, focus on:

```text
a0   first argument, also return value
a1   second argument
a2   third argument
ra   return address
sp   stack pointer
s0   often used as frame pointer when the compiler keeps one
```

### Calling Convention / 调用约定

A calling convention is a contract between caller and callee.

For RV32:

```text
arguments 0-7:  a0-a7
return value:   a0
return address: ra
stack pointer:  sp
frame pointer:  s0/fp, when used
```

The important model is:

```text
caller places first arguments in a0, a1, a2, ...
call writes the return address into ra
callee may save ra and s0/fp on the stack
callee returns an integer value in a0
ret jumps back through ra
```

This differs from a stack-first model. In RV32, the first several integer arguments are normally in registers, not pushed onto the stack.

For `encode_triple(first, second, third)`, expect:

```text
first   -> a0
second  -> a1
third   -> a2
return  -> a0
```

### What Is ELF? / ELF 是什么

`ELF` means:

```text
Executable and Linkable Format
```

ELF is a binary file format used for:

```text
relocatable object files   .o files
executables
shared libraries
kernel-style images
```

In this lab:

```text
build/main.o
```

is an ELF file, but it is not a complete runnable executable. It is a relocatable object file. That means it contains code, symbols, sections, relocations, and debug information, but it has not been fully linked into a final program.

Useful command:

```bash
riscv64-unknown-elf-readelf -h build/main.o
```

What to look for:

```text
Type: REL (Relocatable file)
Machine: RISC-V
Entry point address: 0x0
```

The `Entry point address` is not meaningful yet because `main.o` is not the final executable.

### Sections / ELF 里的 sections

Sections divide an ELF file into different kinds of content.

Useful command:

```bash
riscv64-unknown-elf-readelf -S build/main.o
```

Common sections:

```text
.text       machine instructions
.data       initialized global/static data
.bss        zero-initialized global/static data
.rodata     read-only constants
.debug_*    debug information produced by -g
.symtab     symbol table
.strtab     strings used by the symbol table
```

For this lab, the most important one is:

```text
.text
```

That is where the compiled RV32 instructions live.

### Symbols / ELF 里的 symbols

Symbols are names recorded in the object file.

Useful commands:

```bash
riscv64-unknown-elf-nm build/main.o
riscv64-unknown-elf-readelf -s build/main.o
```

Look for names such as:

```text
main
dispatch_request
encode_triple
g_expected
g_actual
g_done
```

Symbols are why later tools can talk about function names instead of only raw addresses.

### Entry Point / 入口点

An entry point is the address where execution begins in a final executable or kernel image.

Important distinction:

```text
build/main.o      relocatable object file
                  has no meaningful runtime entry point yet

final executable  linked program or kernel image
                  has a meaningful entry point
```

Lab09 stops at the object-file level, so it teaches what an object file contains before final linking.

Week4 QEMU labs will make entry point feel concrete, because QEMU loads a linked ELF file and the CPU begins executing at its entry/start address.

### Stack Frame / 栈帧

With `-O0` and `-fno-omit-frame-pointer`, generated RV32 code is usually easier to read.

In RV32, `s0` is also called `fp` when used as the frame pointer. A typical function may:

```text
move sp down to reserve stack space
save ra and s0/fp
set s0/fp as a stable anchor
restore saved registers before returning
```

Do not expect the first three arguments to appear first on the stack. Start by reading `a0/a1/a2`, then use the stack frame to understand saved registers and locals.

### Bridge to Lab 10 / 过渡到 Lab10

Lab09 teaches:

```text
C compiler generates RV32 instructions
object files contain sections and symbols
function arguments use a0/a1/a2
return values use a0
```

Lab10 adds:

```text
C code can call a function implemented in .S
main.o may refer to a symbol defined in asm_helpers.o
the linker or relocatable combine step connects those symbols
hand-written assembly must follow the same calling convention
callee-saved registers such as s1 must be restored by the callee
```

So the transition is:

```text
Lab09: read compiler-generated RV32 code
Lab10: read hand-written RV32 assembly and check whether it follows the convention
```

## Guided Inspection

Step 1: Build artifacts.

```bash
make
```

This creates the files you will inspect:

```text
build/main.o
build/main.s
build/main.dump
```

Meaning:

```text
build/main.s      assembly text generated by the compiler
build/main.o      ELF relocatable object file
build/main.dump   disassembly of build/main.o
```

Do not look for program output. This lab is about reading build artifacts.

Step 2: Identify the object file type.

```bash
file build/main.o
riscv64-unknown-elf-readelf -h build/main.o
```

What to look for / 看什么:

```text
ELF
RISC-V
relocatable
entry point is not meaningful yet
```

How to understand it / 怎么理解:

```text
ELF:
    The file uses the Executable and Linkable Format.

RISC-V:
    The object file contains RISC-V machine code, not host-machine code.

Relocatable:
    This is a .o file. It can be linked with other object files later.
    It is not a final runnable program yet.

Entry point:
    A final executable or kernel image has an address where execution starts.
    A single .o file usually does not have a meaningful entry point.
```

Useful mental model:

```text
main.c  -> compile ->  main.o
main.o  -> later link with other objects -> final executable or kernel ELF
```

Step 3: Inspect sections.

```bash
riscv64-unknown-elf-readelf -S build/main.o
```

What to look for / 看什么:

```text
.text
.data
.bss
.debug_*
.symtab
```

How to understand it / 怎么理解:

```text
.text:
    Compiled RV32 instructions live here.

.data:
    Initialized global/static data would live here.

.bss:
    Zero-initialized global/static data lives here.

.debug_*:
    Debug information generated by -g lives here.

.symtab:
    The symbol table lives here.
```

You do not need to memorize every section. For this lab, focus on this question:

```text
Where are the instructions?
```

Answer:

```text
.text
```

Step 4: Inspect symbols.

```bash
riscv64-unknown-elf-nm build/main.o
riscv64-unknown-elf-readelf -s build/main.o
```

What to look for / 看什么:

```text
main
dispatch_request
encode_triple
global variables
```

How to understand it / 怎么理解:

Symbols are names that survive into the object file so tools can talk about code and data by name.

Examples:

```text
encode_triple       function symbol
dispatch_request    function symbol
main                function symbol
g_expected          global variable symbol
g_actual            global variable symbol
g_done              global variable symbol
```

Why this matters:

```text
Without symbols, tools would mostly show raw addresses.
With symbols, objdump/readelf/GDB can show names like encode_triple.
```

If `nm` shows letters next to names, read them as a quick symbol map. Common examples:

```text
T    symbol is defined in the text/code section
B    symbol is in zero-initialized global storage
D    symbol is in initialized global storage
U    symbol is undefined in this object and must be provided elsewhere
```

Lab09 mostly has definitions inside one C object. Lab10 will make `U` more important because C will refer to a symbol implemented in `.S`.

Step 5: Inspect generated assembly.

```bash
less build/main.s
```

What to look for / 看什么:

```text
function labels
instructions that write a0, a1, and a2
call encode_triple
```

How to read it / 怎么读:

Search inside `less` with `/dispatch_request`, then look for the call to `encode_triple`.

Around that call, look for instructions that put values into argument registers:

```text
a0    first argument
a1    second argument
a2    third argument
```

The exact instruction sequence can vary, but the meaning should be:

```text
prepare a0
prepare a1
prepare a2
call encode_triple
```

Important habit:

```text
Do not only ask "what does the C source say?"
Ask "what values did the compiler place into the calling-convention registers?"
```

Step 6: Inspect disassembly with relocations.

```bash
riscv64-unknown-elf-objdump -dr build/main.o
```

What to look for / 看什么:

```text
RV32 instructions
relocation entries for calls or addresses
the call to encode_triple
```

How to understand it / 怎么理解:

`build/main.s` is compiler-generated assembly text.

`objdump -dr build/main.o` shows what is inside the object file:

```text
-d    disassemble machine code
-r    show relocation entries
```

Relocation entries exist because `main.o` is not fully linked yet. Some addresses are not final. For example, a call target or global variable address may need to be fixed later by the linker.

Useful mental model:

```text
main.o knows it needs to call encode_triple.
But final addresses are not completely settled until linking.
Relocations tell the linker what must be patched or resolved later.
```

Step 7: Explain the register setup.

Expected explanation:

```text
dispatch_request prepares values for encode_triple.
On RV32, the first three integer arguments go in a0, a1, and a2.
The return value comes back in a0.
```

A stronger explanation:

```text
At the C level, encode_triple has three integer parameters.
At the RV32 ABI level, those parameters are carried by a0, a1, and a2.
The compiler is responsible for generating instructions that put the right
values into those registers before the call.
```

Bridge forward:

```text
In Lab09, the compiler generates the RV32 calling sequence for C.
In Lab10, a hand-written .S file participates in the same convention.
That means the assembly author must know which registers carry arguments,
which register carries the return value, and which registers must be preserved.
```

## Review Questions

1. What does ELF stand for?

   Answer: Executable and Linkable Format.

2. Is `build/main.o` a complete runnable executable?

   Answer: no. It is a relocatable object file.

3. Which RV32 registers carry the first three integer arguments?

   Answer: `a0`, `a1`, and `a2`.

4. Which RV32 register usually carries an integer return value?

   Answer: `a0`.

5. Which RV32 register carries the return address?

   Answer: `ra`.

6. Which ELF section usually contains machine instructions?

   Answer: `.text`.

7. What are symbols useful for?

   Answer: they record names such as functions and global variables so tools can connect names to code/data locations.

8. Why does Lab09 discuss entry point even though `main.o` does not have a meaningful one?

   Answer: because entry point becomes important after linking, especially in Week4 QEMU labs.

9. What does Lab10 add after Lab09?

   Answer: Lab10 adds hand-written `.S` assembly and asks whether that assembly follows the same RV32 calling convention.
