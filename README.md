# GDB ECE391 Labs

Project-oriented debugging labs for preparing for UIUC ECE391.

The goal is not to memorize GDB commands in isolation. Each lab gives you a small broken program, a failure symptom, a guided debugging path, and an explanation of why the same skill matters in ECE391-style systems work.

## Learning Path

- Week 1: GDB basics in user-space C programs.
- Week 2: memory, stack, pointers, and core dumps.
- Week 3: Makefiles, symbols, RISC-V 32 calling convention, and assembly view.
- Week 4: QEMU and GDB remote debugging.
- Week 5: privilege transitions, trap frames, PLIC, and UART interrupts.
- Week 6: kernel threads, Round-Robin scheduling, conditions, locks, and interrupt critical sections.

## How to Start

On your Ubuntu VM:

```bash
sudo apt update
sudo apt install build-essential gdb make binutils
```

For Week 3 RISC-V calling-convention labs, also install:

```bash
sudo apt install gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

For Week 4 QEMU remote-debug labs, also install:

```bash
sudo apt install qemu-system-misc gdb-multiarch
```

Then start with:

```bash
cd week1-gdb-basics/lab01-breakpoints
make
gdb ./build/lab01
```

To compile and smoke-test all completed Week 1 labs:

```bash
bash scripts/verify-week1.sh
```

To compile-check the completed Week 2 labs:

```bash
bash scripts/verify-week2.sh
```

To compile-check the completed Week 3 labs:

```bash
bash scripts/verify-week3.sh
```

To compile-check the completed Week 4 labs:

```bash
bash scripts/verify-week4.sh
```

To build and run all Week 6 solution/exercise checks:

```bash
bash scripts/verify-week6.sh
```

## Why These CFLAGS?

The lab Makefiles use:

```makefile
CFLAGS := -g -O0 -Wall -Wextra -std=c11
```

- `-g`: include debug information so GDB can show source lines, variable names, and function names.
- `-O0`: turn off compiler optimization, making the running program easier to match to the source code.
- `-Wall`: enable common compiler warnings.
- `-Wextra`: enable additional useful warnings beyond `-Wall`.
- `-std=c11`: compile using the C11 language standard.

For debugging, the most important pair is `-g -O0`: give GDB useful debug info and keep the generated code close to the source you are reading.

Week 3 RISC-V labs add target flags such as `-march=rv32im` and `-mabi=ilp32` so generated object files and disassembly match the RV32 calling-convention model used in the lab.

Read each lab in this order:

1. `Goal`
2. `Failure Scenario`
3. `Concept Warmup`
4. `Guided Mode`
5. `Hint Mode`
6. `Review Questions`
7. Optional `Challenge Mode`, if the lab includes one

## Debugging Habit

When you feel stuck, do not stare at the source first. Ask:

- Did it crash, hang, produce the wrong value, or build the wrong binary?
- What is the first command that matches that symptom?
- What does GDB show about the current function, arguments, locals, memory, and registers?

Use [docs/command-cheatsheet.md](docs/command-cheatsheet.md) as the quick lookup table.
