# Lab 11: Runtime RISC-V Registers with QEMU and GDB

## Goal

Lab 10 taught the RV64 calling convention by looking at object files, symbols, and disassembly. Lab 11 uses the same kind of C/assembly example, but now it runs inside QEMU so you can inspect the live CPU state with GDB.

By the end of this lab, you should be able to answer:

- What files does `make` build?
- What is an ELF target?
- What does QEMU load?
- What does GDB read for symbols?
- What does `target remote :1234` do?
- Why do we use `gdb-multiarch`?
- What are `pc`, `sp`, `ra`, `a0`, `a1`, `a2`, and `s1` doing at runtime?
- How does a `.S` file fit into a QEMU-loadable target?

The main sentence to remember:

```text
QEMU loads and runs build/kernel.elf. GDB reads symbols from build/kernel.elf and inspects the live registers.
```

In this lab, the "RV64 target" means this artifact:

```text
build/kernel.elf
```

QEMU uses it as the program loaded into the virtual machine. GDB uses the same file as the symbol file, so addresses can show up as names such as `_start`, `kernel_entry`, and `asm_add_three`.

## Big Picture

Lab 10 was static:

```text
main.c + asm_helpers.S -> object files -> nm/objdump/readelf
```

Lab 11 is runtime:

```text
start.S + main.c + asm_helpers.S -> kernel.elf -> QEMU -> GDB
```

The target code does three useful things:

```text
_start
  sets sp
  calls kernel_entry

kernel_entry in main.c
  calls asm_add_three(100, 20, 1)
  calls check_preserves_s1()

asm_helpers.S
  puts arguments and return values in real RV64 registers
  intentionally lets broken_helper clobber s1
```

The QEMU/GDB relationship is:

```text
                reads symbols
GDB  ------------------------------>  build/kernel.elf
 |
 | remote protocol on :1234
 v
QEMU GDB stub  ---- controls ---->  virtual RV64 CPU
 |
 | loads with -kernel
 v
build/kernel.elf
```

GDB does not run the target as a normal Linux process. QEMU owns the virtual CPU; GDB connects to QEMU and controls that CPU remotely.

## What Does `make` Build?

Run:

```bash
make
make artifacts
```

The build pipeline is:

```text
start.S       -> start.o
main.c        -> main.o
asm_helpers.S -> asm_helpers.o

start.o + main.o + asm_helpers.o + linker.ld
             -> kernel.elf
```

The important artifacts are:

```text
build/start.o
  Tiny entry object. It sets sp, calls kernel_entry, then waits in halt_loop.

build/main.o
  C runtime driver. It calls the assembly helpers and stores observable globals.

build/asm_helpers.o
  Hand-written RV64 assembly helpers from the Lab 10 concept.

build/kernel.elf
  Linked RV64 ELF with symbols. QEMU loads this and GDB reads this.
```

### What Is the Difference Between `.o` and `.elf`?

An `.o` file is an object file: a build-time piece of the final target.

In this lab:

```text
main.c        -> build/main.o
asm_helpers.S -> build/asm_helpers.o
start.S       -> build/start.o
```

Each `.o` file may contain code, data, symbols, debug information, and unresolved references to names defined in other files. For example, `main.o` can refer to `asm_add_three`, while `asm_helpers.o` defines `asm_add_three`.

The `.elf` file is the linked target:

```text
start.o + main.o + asm_helpers.o + linker.ld -> build/kernel.elf
```

The linker combines the object files, resolves symbol references, and uses `linker.ld` to choose the target addresses. In Lab 11, `build/kernel.elf` is the file QEMU loads and the file GDB reads for symbols.

Short version:

```text
.o files are parts.
build/kernel.elf is the assembled target.
```

### What Does `linker.ld` Do?

`linker.ld` is the linker script. It is not code that the CPU executes. It is a layout recipe that tells the linker how to arrange the final `build/kernel.elf`.

The whole file is:

```ld
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;

  .text : {
    KEEP(*(.text.start))
    *(.text*)
  }

  .rodata : {
    *(.srodata*)
    *(.rodata*)
  }

  .data : {
    *(.sdata*)
    *(.data*)
  }

  .bss : {
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(16);
  }
}
```

Read it as:

```text
OUTPUT_ARCH(riscv)
  The output file is for RISC-V.

ENTRY(_start)
  The ELF entry symbol is _start. QEMU begins from this entry point.

SECTIONS { ... }
  This block describes where the final ELF sections go.

. = 0x80000000;
  Set the current output address to 0x80000000. For this QEMU virt target,
  the lab places the first instruction there.

.text
  Code section. The linker puts executable instructions here.

KEEP(*(.text.start))
  Keep the startup code section even if linker garbage collection is enabled.
  This makes sure _start stays at the front of the code layout.

*(.text*)
  Collect all input code sections named .text, .text.foo, and similar from
  start.o, main.o, and asm_helpers.o.

.rodata
  Read-only data section. String literals such as UART messages usually live here.

*(.srodata*) and *(.rodata*)
  Collect small read-only data and regular read-only data from the object files.

.data
  Initialized writable global data.

*(.sdata*) and *(.data*)
  Collect small data and regular initialized data.

.bss
  Zero-initialized or uninitialized global storage. The stack buffer in start.S
  and globals initialized to zero can live here.

*(.sbss*), *(.bss*), and *(COMMON)
  Collect common forms of zero-initialized storage.

. = ALIGN(16);
  Move the end of .bss up to a 16-byte boundary. This keeps the final layout
  neatly aligned for RV64 stack/data expectations.
```

Short version:

```text
linker.ld decides where code and data live in build/kernel.elf.
```

## Is This Like ECE391?

Conceptually, yes:

```text
build target -> run under QEMU -> attach GDB -> inspect CPU state
```

Mechanically, follow the course skeleton.

ECE391 may provide its own Makefile, linker script, QEMU command, kernel layout, device setup, and helper targets. Use the course commands when you are working in the course tree.

This lab uses RV64 because Lab 10 is teaching the RV64 calling convention and we want a direct runtime continuation. In a real course tree, confirm the target width from the provided Makefile and setup scripts. The key transferable ideas are the register roles, symbol lookup, QEMU/GDB connection, and habit of inspecting `pc`, `sp`, `ra`, and argument registers before guessing.

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
- `nm -n` shows symbols such as `_start`, `kernel_entry`, `asm_add_three`, `broken_helper`, and `halt_loop`.
- `objdump -d` shows the instructions GDB can step through at runtime.

For this lab:

```text
QEMU loads the ELF.
GDB reads the ELF symbols.
Your breakpoints use names from the ELF symbol table.
```

## What Is `start.S`?

This lab has two `.S` files:

```text
start.S
asm_helpers.S
```

Uppercase `.S` usually means assembly source that is passed through the C preprocessor before assembly. It is commonly built through GCC:

```bash
riscv64-unknown-elf-gcc -c start.S -o build/start.o
riscv64-unknown-elf-gcc -c asm_helpers.S -o build/asm_helpers.o
```

That matters in OS-style code because assembly files often need constants or macros shared with C.

In this lab:

```text
start.S       provides the entry point and stack setup
asm_helpers.S provides the RV64 calling-convention helper functions
main.c        calls those helper functions at runtime
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

Lab 11 only touches the first layer: CPU instructions, registers, ELF symbols, UART output, and the QEMU/GDB connection.

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
- `set architecture riscv:rv64`: tell GDB this lab's target is RV64.
- `target remote :1234`: connect to QEMU's GDB remote stub.

Now GDB can:

- inspect live registers such as `pc`, `sp`, `ra`, `a0`, `a1`, `a2`, and `s1`
- stop at C symbols such as `kernel_entry`
- stop at assembly symbols such as `asm_add_three` and `broken_helper`
- step one machine instruction at a time with `si`
- compare runtime register state with the static Lab 10 disassembly

### Why `gdb-multiarch` Instead of `gdb`?

Your development machine and this lab's target may use different CPU architectures. The target here is a freestanding RV64 machine inside QEMU.

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

If you are using a course VM or container, prefer the course setup instructions if they differ.

Step 2: Build the runtime target.

```bash
make
make artifacts
```

What to look for: the artifact map should show `start.S`, `main.c`, and `asm_helpers.S` all becoming part of `build/kernel.elf`.

Step 3: Inspect the ELF before running it.

```bash
make inspect
```

What to look for:

- `kernel.elf` is a RISC-V ELF file.
- `_start`, `kernel_entry`, `asm_add_three`, `check_preserves_s1`, `broken_helper`, and `halt_loop` appear in the symbol table.
- `objdump` shows instructions from both sources:

  ```text
  kernel_entry
    compiled by GCC from main.c

  asm_add_three
  check_preserves_s1
  broken_helper
    assembled from asm_helpers.S
  ```

If you run a full disassembly yourself, the helper functions may not appear in the first screen of output. Search for their names or disassemble one symbol directly:

```bash
riscv64-unknown-elf-objdump -d build/kernel.elf --disassemble=asm_add_three
riscv64-unknown-elf-objdump -d build/kernel.elf --disassemble=broken_helper
```

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

This is a normal run, so there is no GDB connection yet. The virtual CPU starts immediately.

What to look for: serial output should include:

```text
Lab 11 runtime target reached
asm_add_three returned 121
check_preserves_s1 detected clobbered s1
Lab 11 done; CPU will wait in halt_loop
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

Step 7: Inspect the first CPU state.

```gdb
info registers pc sp ra
x/i $pc
```

What to look for:

- `pc` is the current instruction address.
- Immediately after connecting, the CPU is paused at the beginning of the target, so `sp` may not be interesting yet.
- After `_start` runs, `sp` points at the stack region defined in `start.S`.
- `ra` changes when one function calls another.

Step 8: Watch C pass arguments into assembly.

```gdb
break asm_add_three
continue
info registers a0 a1 a2 ra sp
x/3i $pc
```

What to look for:

```text
a0 = 100
a1 = 20
a2 = 1
```

Those are the three C arguments from:

```c
asm_add_three(100, 20, 1)
```

Now step through the two add instructions:

```gdb
si
info registers a0
si
info registers a0
```

What to look for:

```text
after first add:  a0 = 120
after second add: a0 = 121
```

That is the runtime version of the static Lab 10 observation: arguments enter in `a0/a1/a2`, and the integer return value comes back in `a0`.

Step 9: Connect the return value to C state.

```gdb
break runtime_after_add
continue
p g_add_result
```

What to look for:

```text
g_add_result = 121
```

This proves the value returned from assembly became a C global.

Step 10: Watch the callee-saved register bug happen.

```gdb
break broken_helper
break bad_preserve
break runtime_after_preserve_check
continue
```

At `broken_helper`, inspect `s1` before the bad instruction changes it:

```gdb
info registers s1 a0 ra sp
x/3i $pc
```

What to look for:

```text
s1 = 0x39139139
```

Now execute one instruction:

```gdb
si
info registers s1
```

What to look for:

```text
s1 = 0x0badc0de
```

That is the bug from Lab 10, but now you watched it happen on the live virtual CPU.

Continue to the failure branch:

```gdb
continue
x/i $pc
```

What to look for: GDB stops at `bad_preserve`, proving `check_preserves_s1` noticed that `s1` did not survive the call.

Continue to the C symbol `runtime_after_preserve_check`. This is a small marker function in `main.c` that gives GDB a clean place to stop after `g_check_result` has been written:

```gdb
continue
p g_check_result
```

What to look for:

```text
g_check_result = 1
```

Step 11: Let the target finish.

```gdb
continue
```

What to look for: QEMU prints the Lab 11 messages, then the CPU waits in `halt_loop`.

## Review Questions

1. Which Lab 11 file does QEMU load?

   Answer: `build/kernel.elf`.

2. Which Lab 11 file does GDB use for symbols?

   Answer: `build/kernel.elf`.

3. What does `target remote :1234` do?

   Answer: it connects GDB to QEMU's remote debugging stub on TCP port `1234`.

4. What does QEMU's `-S` option do?

   Answer: it starts the virtual CPU paused.

5. What does QEMU's `-s` option do?

   Answer: it opens the default GDB remote debugging port, `1234`.

6. Which registers carry `asm_add_three(100, 20, 1)` at runtime?

   Answer: `a0 = 100`, `a1 = 20`, and `a2 = 1`.

7. Which register carries the integer return value from `asm_add_three`?

   Answer: `a0`.

8. Which register shows the current instruction address?

   Answer: `pc`.

9. Why is changing `s1` inside `broken_helper` a bug?

   Answer: `s1` is callee-saved, so a function that changes it must restore it before returning.

10. What is `.S`?

    Answer: assembly source that is usually passed through the C preprocessor and assembled through GCC.

11. What is the main Lab 10 to Lab 11 bridge?

    Answer: Lab 10 inspects the calling convention statically; Lab 11 watches the same register rules happen at runtime in QEMU through GDB.
