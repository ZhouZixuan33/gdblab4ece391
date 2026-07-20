# Lab 12 Bare-Metal Entry and Raw Breakpoint Design

## Purpose

Revise Week 4 Lab 12 so that `break *0x80000000` is not a command learners
copy without context. The lab will explain the difference between a normal
Linux process and a bare-metal target, identify the RISC-V privilege mode used
by this QEMU configuration, and trace the target from QEMU reset through
`_start`, `kernel_entry`, and C `main`.

## Learning Outcomes

After the lab, a learner should be able to explain and demonstrate:

1. The intended roles of RISC-V M-mode, S-mode, and U-mode: initial machine
   startup, an operating-system kernel, and protected user programs.
2. Why a normal Linux C program usually runs in U-mode, while this lab starts
   and remains in M-mode.
3. Why this lab does not yet perform an M-to-S or S-to-U transition, and why an
   OS may execute initial boot code in M-mode before its main S-mode kernel.
4. Why neither the `.c` suffix nor the name `main` determines privilege mode.
5. What `-bios none` removes from the boot path and what startup work remains.
6. How `linker.ld`, the ELF entry, QEMU, and GDB agree on `0x80000000`.
7. Why the QEMU `virt` reset location and the lab's linked entry address are
   different.
8. What `_start`, `kernel_entry`, and `main` each contribute.
9. How symbolic and raw-address breakpoints provide complementary evidence.
10. Why QEMU loads `build/kernel.elf` while GDB reads the same ELF for symbols
   and debug information, but GDB controls the live CPU through QEMU's remote
   stub.
11. How to configure RV64 in GDB, connect with `target remote :1234`, and verify
   the loaded target with `info files` and `info functions`.
12. How to set, list, hit, continue between, and delete multiple breakpoints.
13. How to prove that execution reached `_start`, `kernel_entry`, `main`,
    `init_console`, and `debug_checkpoint` instead of inferring progress only
    from UART output.
14. Why a raw breakpoint is useful when symbols are absent, stale, suspicious,
    or loaded with the wrong address assumptions.

## Scope

Lab 12 will introduce U-mode, S-mode, and M-mode and compare process startup
with bare-metal startup. It will not implement an `mret`/`sret` transition,
create a U-mode process, add virtual memory, or implement system calls.

The README will explicitly call the target "kernel-style/bare-metal code" and
will not imply that a function named `kernel_entry` changes CPU privilege.

Lab 11 already introduces QEMU and GDB remote control. Lab 12 will briefly
recap that mechanism, then use it as a tool for gathering evidence about entry
addresses and control flow rather than reteaching the entire connection setup.

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

## Retained Remote-Debugging Objectives

The lab will use neutral guided-verification goals. UART messages show visible
progress, while static ELF inspection and live GDB state provide stronger
evidence about the precise instruction, function, address, and register state
the CPU reached.

The guided exercise will preserve this progression:

1. Build the ELF and inspect its entry, symbols, and disassembly.
2. Start QEMU with `-s -S` in terminal 1.
3. Start GDB with `build/kernel.elf` in terminal 2.
4. Select `riscv:rv64` and connect with `target remote :1234`.
5. Use `info files` and `info functions` to confirm that GDB has the expected
   ELF and named symbols.
6. Set symbolic breakpoints on `kernel_entry` and `debug_checkpoint`, list them
   with `info breakpoints`, and use `continue` to observe their order.
7. Inspect `pc`, `sp`, `ra`, and the instruction at `$pc` at each meaningful
   stop.
8. Restart the target, delete the symbolic breakpoints, and use
   `break *0x80000000` to validate the entry independently of the `_start`
   symbol name.

The README will continue to teach both breakpoint forms:

```gdb
break kernel_entry       # resolve a name through the ELF symbol table
break *0x80000000        # insert a breakpoint at an instruction address
```

It will also preserve the distinction between the two roles of the ELF:

```text
QEMU reads load addresses and machine code from build/kernel.elf.
GDB reads symbols and debug information from build/kernel.elf.
GDB uses the remote protocol to inspect and control QEMU's live CPU.
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

`init_console` remains a named, non-inlined function so the old symbol-learning
objective and checkpoint sequence are retained in the C implementation.

No C runtime assumptions will be introduced: no libc, heap, command-line
arguments, constructors, or implicit process exit. The existing freestanding
compiler and linker flags remain in force.

The README will explain that `-ffreestanding`, `-nostdlib`, and `-nostartfiles`
remove the hosted assumptions that normally surround a C `main`. It will also
distinguish a program image from an OS-managed process and explain the RV64 ABI
requirements at the assembly-to-C boundary: a valid aligned stack, `ra` set by
`call`, and integer return values in `a0`.

## UART and MMIO Boundary

UART is supporting context rather than a second lab topic. The README will use
the two prominent addresses to contrast RAM and device MMIO:

```text
0x80000000 -> instructions in RAM, beginning at _start
0x10000000 -> QEMU virt NS16550A-compatible UART registers
```

The C code will use a `volatile` byte pointer, poll the line-status transmitter
ready bit, and write characters to the transmit register. The README will
explain why `printf` is unavailable, what MMIO and `volatile` mean, and why the
implementation is polling rather than interrupt-driven.

`init_console` is a named startup boundary and debugger checkpoint, not a full
UART driver. The text will say that QEMU presents the UART in a state suitable
for this minimal polling example. Register configuration, PLIC routing, UART
interrupts, buffering, and a production driver interface remain out of scope.

## ECE391 Bridge

The final conceptual bridge will show how this lab's M-mode-only path prepares
for the course sequence:

```text
Lab 12:  M-mode reset -> _start -> kernel_entry -> main
ECE391:  initial boot -> S-mode kernel -> ELF/user context -> sret -> U-mode
```

The lab will mention that privilege transitions require architectural
mechanisms such as traps and `mret`/`sret`; a C call and a function name never
change privilege. It will not expand into OpenSBI internals, page tables,
device-tree parsing, system calls, or an interrupt-driven UART.

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
`kernel_entry`, `main`, `init_console`, and `debug_checkpoint`. They will also
verify the symbol file, manage multiple breakpoints, and inspect `pc`, `sp`,
and `ra` at meaningful stops.

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
