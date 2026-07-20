# Lab 12 Bare-Metal Entry and Raw Breakpoint Design

## Purpose

Revise Week 4 Lab 12 so that `break *0x80000000` is not a command learners
copy without context. The lab will explain the difference between a normal
Linux process and a bare-metal target, identify the RISC-V privilege mode used
by this QEMU configuration, and trace the target from QEMU reset through
`_start`, `kernel_entry`, and C `main`.

## Learning Outcomes

After the lab, a learner should be able to explain:

1. Why a normal Linux C program usually runs in U-mode, while this lab runs in
   M-mode.
2. Why neither the `.c` suffix nor the name `main` determines privilege mode.
3. What `-bios none` removes from the boot path and what startup work remains.
4. How `linker.ld`, the ELF entry, QEMU, and GDB agree on `0x80000000`.
5. Why the QEMU `virt` reset location and the lab's linked entry address are
   different.
6. What `_start`, `kernel_entry`, and `main` each contribute.
7. How symbolic and raw-address breakpoints provide complementary evidence.

## Scope

Lab 12 will introduce U-mode, S-mode, and M-mode and compare process startup
with bare-metal startup. It will not implement an `mret`/`sret` transition,
create a U-mode process, add virtual memory, or implement system calls.

The README will explicitly call the target "kernel-style/bare-metal code" and
will not imply that a function named `kernel_entry` changes CPU privilege.

## Conceptual Model

The central comparison is:

```text
Normal Linux process                    Lab 12 bare-metal target
--------------------                    ------------------------
OS creates a U-mode process             QEMU resets a virtual machine in M-mode
OS loader prepares an ELF image         -bios none omits platform firmware
C runtime _start prepares the process   QEMU reset code jumps to the ELF entry
C runtime calls main                    our _start sets sp
main normally remains in U-mode         kernel_entry calls main in M-mode
```

The lab will state that source language and function names do not select a
privilege mode. A C function may execute in U-, S-, or M-mode depending on the
execution environment and the CPU's current privilege state.

## Entry Address Evidence Chain

The code layout will make the following equality intentional and verifiable:

```text
linker .text start == ELF entry == address of _start == 0x80000000
```

The implementation will:

- place `_start` in `.text.start`;
- use `KEEP(*(.text.start))` before other `.text` input sections;
- retain `ENTRY(_start)`;
- begin the linked image at `0x80000000`.

The README will distinguish the QEMU `virt` reset vector near `0x1000` from the
ELF entry at `0x80000000`. With `-S`, GDB may initially observe the reset code.
After `continue`, `break *0x80000000` catches the boundary where execution first
reaches the lab's own instructions.

The learner will verify the chain with:

```text
readelf -h  -> ELF entry point
nm -n       -> _start symbol address
objdump -d  -> instruction at that address
GDB $pc     -> live execution reaches that address
```

## Startup Code Structure

`start.S` will contain only the assembly-specific startup responsibilities:

```text
_start
  initialize sp from stack_top
  call kernel_entry
  enter halt_loop if kernel_entry returns
```

`main.c` will contain the C-side flow:

```text
kernel_entry
  call init_console
  call main
  return main's result

main
  print the entry message
  call debug_checkpoint
  call scheduler_checkpoint
  print the completion message
  return
```

The checkpoint functions and UART output will move from assembly to C so the
learner can see that ordinary C functions can execute in the bare-metal M-mode
environment. Observable state variables will remain available for GDB.

No C runtime assumptions will be introduced: no libc, heap, command-line
arguments, constructors, or implicit process exit. The existing freestanding
compiler and linker flags remain in force.

## Files to Change

- `lab12-remote-breakpoints/README.md`: replace the short explanation with the
  process-vs-bare-metal comparison, privilege-mode boundary, entry-address
  evidence chain, and guided breakpoint observations.
- `lab12-remote-breakpoints/start.S`: reduce to stack setup, call into C, and
  halt; use `.text.start`.
- `lab12-remote-breakpoints/main.c`: add `kernel_entry`, `main`, UART helpers,
  named checkpoints, and observable state.
- `lab12-remote-breakpoints/linker.ld`: keep startup code first and collect
  small data/read-only/BSS sections emitted by the C compiler.
- `lab12-remote-breakpoints/Makefile`: compile `main.c`, link `main.o`, and add
  inspection commands where they improve the guided evidence chain.
- `scripts/verify-week4.sh`: update only if the new artifact or symbol layout
  requires corresponding verification.

## Verification

Static verification must prove:

- the Lab 12 target builds without a hosted C runtime;
- the ELF entry is `0x80000000`;
- `_start` resolves to `0x80000000`;
- `_start`, `kernel_entry`, `main`, `debug_checkpoint`, and
  `scheduler_checkpoint` exist;
- disassembly shows `_start` calling `kernel_entry`, and C-side call flow is
  present;
- the repository Week 4 verification script succeeds.

Interactive instructions will ask the learner to confirm the initial PC,
continue to the raw breakpoint, compare it with `_start`, then continue through
`kernel_entry`, `main`, and `debug_checkpoint`.

## Documentation Guardrails

The revised lab must not say:

- that all C code runs in U-mode;
- that `main` is inherently a user-mode entry point;
- that `kernel_entry` changes privilege because of its name;
- that `0x80000000` is the RISC-V reset vector;
- that `ENTRY(_start)` alone controls section placement;
- that `-bios none` removes QEMU's entire reset process.

Instead, each claim will be tied to a file, command output, or live GDB
observation the learner can reproduce.
