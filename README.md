# ECE391 Systems Preparation Labs

A progressive, hands-on introduction to the systems concepts used in UIUC
ECE391.

This repository is designed for students encountering ECE391-style systems
programming for the first time. It begins with small user-space programs and
gradually moves toward bare-metal RISC-V kernels, interrupts, threads,
synchronization, virtual memory, processes, and system calls.

GDB and QEMU are not the main subjects of the project. They are diagnostic
tools: GDB exposes program state, and QEMU provides a controlled machine on
which low-level behavior can be observed. The real goal is to learn how a
system works, how it fails, and how to reason from symptoms to root causes.

> [!NOTE]
> This is an independent study project. It is not an official UIUC or ECE391
> repository.

## What You Will Learn

By following the curriculum in order, you will practice how to:

- inspect C programs, stack frames, pointers, memory, registers, and assembly;
- understand Makefiles, symbols, ELF files, linking, and the RISC-V calling
  convention;
- debug bare-metal programs through QEMU's remote GDB interface;
- follow privilege transitions and reason about CSRs, traps, and interrupts;
- implement and inspect context switches, schedulers, thread lifecycles, and
  synchronization;
- build toward virtual memory, processes, and system calls.

The labs emphasize a reusable systems-debugging habit:

```text
symptom -> hypothesis -> machine state -> root cause -> systems explanation
```

## Curriculum

Follow the weeks in order if this is your first exposure to ECE391.

| Week | Topic | What it prepares you to reason about | Status |
| --- | --- | --- | --- |
| [1](week1-gdb-basics/README.md) | Debugging fundamentals | Breakpoints, stepping, stack frames, and watchpoints | Complete |
| [2](week2-memory-debugging/README.md) | Memory debugging | Array bounds, pointer lifetime, heap failures, and core dumps | Complete |
| [3](week3-build-asm-32bit/README.md) | Builds, ELF, and assembly | Makefiles, stale builds, symbols, RISC-V calling conventions, and mixed C/assembly | Complete |
| [4](week4-qemu-remote-gdb/README.md) | Bare-metal debugging | QEMU, remote GDB, startup code, registers, and exceptions | Complete |
| [5](week5-csr-interrupt-trap/) | Traps and interrupts | Privilege transitions, CSRs, trap frames, PLIC, and UART interrupts | Complete |
| [6](week6-thread/README.md) | Kernel threads | Context switching, scheduling, lifecycle states, locks, and interrupt-safe critical sections | Complete |
| [7](week7-virtual-memory/) | Virtual memory | Address translation, page tables, and memory protection | **In Progress — lecture material only** |
| [8](week8-process-syscall/) | Processes and system calls | User/kernel boundaries, process state, and syscall handling | **In Progress — lecture material only** |

Weeks 7 and 8 are part of the intended learning path, but their runnable labs
have not been added yet.

## Prerequisites

You should be comfortable with:

- basic C syntax, functions, arrays, pointers, and structs;
- using a terminal and navigating directories;
- binary and hexadecimal notation;
- running `make`.

You do not need prior kernel-development experience. The early weeks establish
the debugging and machine-level vocabulary needed by the later labs.

## Environment Setup

The supported environment is Ubuntu 24.04 LTS on x86-64.

Install the native tools used by the first two weeks:

```bash
sudo apt update
sudo apt install build-essential gdb make binutils
```

Install the 32-bit, RISC-V, and emulation tools used by later weeks:

```bash
sudo apt install gcc-multilib libc6-dev-i386 \
  qemu-system-misc gcc-riscv64-unknown-elf \
  binutils-riscv64-unknown-elf gdb-multiarch
```

See [Ubuntu 24.04 setup](setup/ubuntu-24.04.md) for verification commands and
[package notes](setup/packages.md) for an explanation of what each package
provides.

## Start Here

Clone or enter the repository, then begin with the first lab:

```bash
cd week1-gdb-basics/lab01-breakpoints
make
gdb ./build/lab01
```

Inside GDB:

```gdb
break main
run
next
print result
```

Read the lab's `README.md` while you work. It explains the failure scenario,
the observations to make, and the questions you should be able to answer.

## How to Work Through a Lab

Use the same loop for each lab:

1. Read the goal and failure scenario.
2. Predict which program or machine state should explain the symptom.
3. Build the target and reproduce the behavior.
4. Inspect the relevant source, stack, memory, registers, or control flow.
5. Explain the root cause in systems terms—not only as a debugger command.
6. Complete the review questions and optional challenge.
7. Run the lab or week check when one is provided.

Early labs run as normal Linux programs. Later labs build freestanding RISC-V
code and commonly provide targets such as:

```bash
make MODE=solution run-check
make MODE=exercise
make MODE=solution debug
make MODE=solution gdb
```

Available targets vary by lab, so treat each lab README and Makefile as the
source of truth.

## Verify Completed Weeks

From the repository root:

```bash
bash scripts/verify-week1.sh
bash scripts/verify-week2.sh
bash scripts/verify-week3.sh
bash scripts/verify-week4.sh
bash scripts/verify-week6.sh
```

These scripts compile or exercise the material currently covered by automated
checks. There is no repository-wide Week 5 verification script yet; use the
checks documented inside its individual labs.

## Reference Material

- [Command cheatsheet](docs/command-cheatsheet.md): quick lookup for common
  GDB, ELF, and inspection commands.
- [ECE391 debugging map](docs/ece391-debugging-map.md): connects debugging
  observations to course-style systems concepts.
- [Terminology](docs/terminology.md): definitions used throughout the labs.
- [`setup/`](setup/): environment and package guidance.

Lecture PDFs stored with some weeks are course study references. Course names
and materials remain the property of their respective authors and
institutions.

## A Note on Debugging Flags

User-space labs generally compile with:

```makefile
CFLAGS := -g -O0 -Wall -Wextra -std=c11
```

`-g` preserves source-level debug information, while `-O0` keeps generated code
close to the source being studied. `-Wall` and `-Wextra` expose suspicious code
early, and `-std=c11` selects the C language version.

Later RISC-V labs add architecture and ABI flags such as `-march=rv32im` and
`-mabi=ilp32`, or their RV64 equivalents, according to the target being built.
