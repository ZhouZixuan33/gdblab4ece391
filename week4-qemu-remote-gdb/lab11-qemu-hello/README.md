# Lab 11: RISC-V QEMU, ELF Symbols, and First Remote GDB Session

## Goal

Build the mental model for ECE391-style QEMU/GDB debugging on a RISC-V target.

By the end of this lab, you should be able to answer:

- What files does `make` build?
- What is an ELF file?
- What does QEMU run?
- What does GDB read for symbols?
- What does `target remote :1234` do?
- Why do we use `gdb-multiarch`?
- What are `pc`, `sp`, `ra`, and `a0`?
- How do `.S` files relate to assembly in ECE391-style code?

This lab is not a bug hunt. It is the conceptual base for the rest of Week 4.

The main sentence to remember:

```text
QEMU runs the RISC-V target. GDB reads the ELF symbols.
```

In this lab, both of those point at the same file:

```text
build/kernel.elf
```

## Big Picture

In a normal user-space GDB session, you might run:

```bash
gdb ./build/program
```

GDB starts that Linux process itself.

In this QEMU session, the pieces are different:

```text
                reads symbols
GDB  ------------------------------>  build/kernel.elf
 |
 | remote protocol on :1234
 v
QEMU GDB stub  ---- controls ---->  virtual RISC-V CPU
 |
 | loads with -kernel
 v
build/kernel.elf
```

Meaning:

- QEMU emulates a RISC-V `virt` machine.
- QEMU loads `build/kernel.elf` with `-kernel`.
- GDB also opens `build/kernel.elf` so names like `_start` and `kernel_entry` make sense.
- GDB connects to QEMU through TCP port `1234`.
- GDB does not run the kernel as a normal Linux process.

## What Does `make` Build?

Run:

```bash
make
make artifacts
```

The build pipeline is:

```text
start.S       -> start.o
start.o
+ linker.ld   -> kernel.elf
kernel.elf    -> kernel.bin
```

The important artifacts are:

```text
build/start.o
  RISC-V object file assembled from start.S.

build/kernel.elf
  Linked RISC-V ELF with symbols. QEMU loads this and GDB reads this.

build/kernel.bin
  Flat bytes extracted from kernel.elf for comparison. Lab 11 does not boot this file.
```

`start.S` is the small RISC-V assembly source for this lab. The Makefile turns it into an object file, then links it into `kernel.elf`.

For RISC-V QEMU `virt`, using `-kernel build/kernel.elf` is the clean mental model students need: there is a target ELF, QEMU loads it, and GDB uses its symbols.

## What Is a Bootable Target?

In this lab, "bootable target" does not mean a hand-made disk image with a BIOS boot sector.

It means:

```text
an artifact QEMU knows how to load and start
```

Here, that artifact is:

```text
build/kernel.elf
```

QEMU starts it with:

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf
```

Option meanings:

```text
qemu-system-riscv64
  Emulate a 64-bit RISC-V machine.

-machine virt
  Use QEMU's generic RISC-V virtual board.

-nographic
  Use the terminal for serial I/O instead of opening a GUI window.

-bios none
  Do not boot through a firmware image; start the kernel target directly.

-kernel build/kernel.elf
  Load this ELF as the target program.
```

### Is This Like ECE391?

Conceptually, yes:

```text
build target -> run under QEMU -> attach GDB -> inspect CPU state
```

Mechanically, follow the course skeleton.

ECE391 may provide its own Makefile, linker script, QEMU command, kernel layout, device setup, and helper targets. You should not invent your own QEMU command if the course already gives one.

This lab uses a 64-bit RISC-V target because its local Makefile uses `qemu-system-riscv64`, `riscv64-unknown-elf-*`, `-march=rv64imac`, and GDB architecture `riscv:rv64`. Do not infer from this lab alone that ECE391 must always use 64-bit RISC-V. In the course code, confirm the target width from the provided Makefile and setup scripts.

For ECE391 preparation, you need to understand:

- QEMU runs a virtual machine target.
- The target has an entry point.
- GDB needs symbols to map addresses to names.
- The current instruction address is in `pc`.
- The stack pointer is `sp`.
- If there is no output, inspect registers and current instruction before guessing.

## What Is ELF?

ELF means Executable and Linkable Format.

An ELF file can contain:

```text
machine code
data
sections
symbols
debug information
entry point metadata
```

Use:

```bash
make inspect
```

Or manually:

```bash
file build/kernel.elf
riscv64-unknown-elf-readelf -h build/kernel.elf
riscv64-unknown-elf-nm -n build/kernel.elf
riscv64-unknown-elf-objdump -d build/kernel.elf
```

What to notice:

- `file` identifies `kernel.elf` as a RISC-V ELF.
- `readelf -h` shows the ELF header and entry point.
- `nm -n` shows symbol names and addresses.
- `objdump -d` shows RISC-V instructions under symbols such as `_start` and `kernel_entry`.

### Does ECE391 Use ELF?

Yes, ELF is part of the systems-programming world around kernels, loaders, and debuggers.

For debugging:

```text
GDB often needs a symbol-bearing ELF to turn addresses into names.
```

For program loading:

```text
Course code may discuss executable headers, magic numbers, entry points,
and loading program segments.
```

You do not need a full ELF loader for Lab 11. You need to know:

```text
ELF is structured and symbol-rich.
GDB can use ELF symbols.
QEMU can load this RISC-V ELF with -kernel.
```

## What Is `.S`?

ECE391-style low-level code uses assembly, but the file extension and assembler matter.

This lab uses:

```text
start.S
```

Uppercase `.S` usually means assembly source that is passed through the C preprocessor before assembly. It is commonly built through GCC:

```bash
riscv64-unknown-elf-gcc -c start.S -o build/start.o
```

That matters because `.S` files can use preprocessor features:

```asm
#include "some_header.h"
#define SOME_CONSTANT 123
```

This is common in OS skeleton code where C and assembly need shared constants.

The pipeline is:

```text
assembly source -> object file -> linked ELF/kernel target -> QEMU -> GDB
```

## What Is QEMU Doing?

QEMU is emulating a RISC-V machine:

```text
virtual CPU
virtual memory
virtual UART/serial device
virtual interrupt controller/device model
GDB remote debugging stub
```

The course note you quoted mentions RISC-V manuals, PLIC specs, and VirtIO specs. Those are not assembly syntax manuals. They describe the machine and devices your OS code talks to:

```text
RISC-V instruction set manual:
  registers, instructions, traps, privilege, CSRs

PLIC specification:
  platform interrupt controller behavior

VirtIO specification:
  virtual device interface used by emulators such as QEMU
```

Lab 11 only touches the first layer: CPU instructions, registers, ELF symbols, and QEMU/GDB connection. Later labs can build toward traps, interrupts, and device debugging.

## What Is GDB Doing?

When you start QEMU for debugging:

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf -s -S
```

The new options are:

```text
-s
  Open QEMU's built-in GDB remote stub on TCP port 1234.

-S
  Start the virtual CPU paused.
```

Then in another terminal, start GDB manually:

```bash
gdb-multiarch build/kernel.elf
```

Inside GDB:

```gdb
set architecture riscv:rv64
target remote :1234
```

Meaning:

- `gdb-multiarch build/kernel.elf`: start GDB and load symbols from the ELF.
- `set architecture riscv:rv64`: tell GDB this lab's target is 64-bit RISC-V.
- `target remote :1234`: connect to QEMU's GDB remote stub.

In ECE391, use the architecture and GDB setup from the course skeleton. If the course target is different, this line changes with it.

Now GDB can:

- read symbols from `kernel.elf`
- pause and continue the virtual CPU
- inspect registers such as `pc`, `sp`, `ra`, and `a0`
- set breakpoints such as `break kernel_entry`
- examine machine instructions with `x/i $pc`

The Makefile has a convenience target:

```bash
make gdb
```

Use it after you understand the manual commands. ECE391-style course Makefiles may not provide an equivalent helper.

### Why `gdb-multiarch` Instead of `gdb`?

Your development machine and this lab's target may use different CPU architectures. The target here is a freestanding RISC-V machine inside QEMU.

`gdb-multiarch` is a safer default because it supports multiple CPU architectures and remote targets.

Plain `gdb` may work if it was built with RISC-V target support, but do not assume that. For this lab, prefer:

```bash
gdb-multiarch build/kernel.elf
```

## Guided Mode

Step 1: Check tools.

```bash
make check-tools
```

What to look for: `riscv64-unknown-elf-gcc`, RISC-V binutils, `qemu-system-riscv64`, and GDB should be available.

If the check reports missing tools on Ubuntu or WSL, install the Week 4 toolchain:

```bash
sudo apt update
sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make
```

Then rerun:

```bash
make check-tools
```

If you are using a course VM or container, prefer the course setup instructions if they differ. The package names above show the capabilities this lab expects: RISC-V cross compiler, RISC-V binutils, QEMU, GDB, and Make.

Step 2: Build the artifacts.

```bash
make
make artifacts
```

What to look for: the artifact map should show that `kernel.elf` is the key file for both QEMU and GDB.

Step 3: Inspect the ELF.

```bash
make inspect
```

What to look for:

- `kernel.elf` is a RISC-V ELF file.
- `_start`, `kernel_entry`, and `hello_checkpoint` appear in the symbol table.
- `objdump` shows RISC-V instructions.

Step 4: Run the target normally with QEMU.

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf
```

Read the command left to right:

```text
qemu-system-riscv64
  Start QEMU's full-system emulator for a 64-bit RISC-V machine.
  This creates a virtual machine target; it does not run kernel.elf as a
  normal program on your host OS.

-machine virt
  Use QEMU's generic RISC-V virtual board. This board gives the target
  a CPU, RAM, and memory-mapped devices. The UART address used by this
  lab belongs to this virtual board.

-nographic
  Do not open a separate graphical QEMU window. Connect the virtual
  serial console to this terminal instead. That is why UART output
  appears in the terminal where you started QEMU.

-bios none
  Do not start a firmware image first. For this lab, QEMU loads the
  kernel target directly.

-kernel build/kernel.elf
  Load this ELF file into the virtual machine. The ELF header and linker
  script tell QEMU where the target code lives and where execution starts.
```

This is a normal run, so there is no GDB connection yet. The virtual CPU starts immediately. Step 5 adds `-s -S` when you want QEMU to wait for GDB.

What to look for: serial output should include:

```text
Lab 11 kernel_entry reached
Lab 11 hello_checkpoint reached
```

After the messages print, the target waits in `halt_loop`. To quit QEMU in `-nographic` mode, press `Ctrl-a`, then `x`.

After you understand the QEMU command, you can use:

```bash
make run
```

Step 5: Start QEMU paused for GDB.

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf -s -S
```

What to look for: QEMU waits. That is expected because `-S` pauses the virtual CPU until GDB sends `continue`.

After you understand the debug command, you can use:

```bash
make debug
```

Step 6: Open a second terminal and start GDB manually.

```bash
gdb-multiarch build/kernel.elf
```

Inside GDB:

```gdb
set architecture riscv:rv64
target remote :1234
```

Step 7: Inspect where the virtual CPU is.

```gdb
info registers
x/i $pc
```

What to look for: `pc` is the program counter, the RISC-V current-instruction address.

Step 8: Break at the target entry points.

```gdb
break _start
break kernel_entry
continue
```

What to look for: GDB should stop when QEMU reaches these symbols.

Step 9: Connect symbols to CPU state.

```gdb
info registers pc sp ra a0
x/i $pc
break hello_checkpoint
continue
```

What to look for: the current `pc` should match code that GDB can describe with symbols from `kernel.elf`.

Step 10: Let the target finish.

```gdb
continue
```

What to look for: QEMU prints the Lab 11 messages, then the CPU waits in `halt_loop`.

## Review Questions

1. Which Lab 11 file does QEMU load?

   Answer: `build/kernel.elf`.

2. Which Lab 11 file does GDB use for symbols?

   Answer: `build/kernel.elf`.

3. What is the short memory sentence for QEMU and GDB?

   Answer:

   ```text
   QEMU runs the RISC-V target. GDB reads the ELF symbols.
   ```

4. What does `target remote :1234` do?

   Answer: it connects GDB to QEMU's remote debugging stub on TCP port `1234`.

5. What does QEMU's `-S` option do?

   Answer: it starts the virtual CPU paused.

6. What does QEMU's `-s` option do?

   Answer: it opens the default GDB remote debugging port, `1234`.

7. Which RISC-V register shows the current instruction address?

   Answer: `pc`, the program counter.

8. Which RISC-V register is the stack pointer?

   Answer: `sp`.

9. Why does this lab recommend `gdb-multiarch` instead of plain `gdb`?

   Answer: the target is RISC-V, and `gdb-multiarch` is the safer default for cross-architecture remote debugging.

10. What is `.S`?

    Answer: assembly source that is usually passed through the C preprocessor and assembled through GCC.

11. What ELF knowledge matters most for this lab?

    Answer: know that ELF is structured and symbol-rich, and tools such as GDB, `nm`, `readelf`, and `objdump` can use it to map addresses to names and instructions.
