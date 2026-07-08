# Week 4: QEMU and GDB Remote Debugging

Week 4 moves from local Linux processes into a tiny RISC-V QEMU target. The goal is to practice the debugging workflow used before and during ECE391-style kernel work:

```text
start QEMU -> connect GDB remotely -> load symbols -> inspect registers,
addresses, stack state, disassembly, and QEMU logs
```

These labs stay lightweight. They use a tiny RISC-V `virt` target so you can learn the debugging motions without building a full operating system.

## Labs

- `lab11-qemu-hello`: start a RISC-V QEMU target paused for GDB, connect with `target remote :1234`, and continue execution.
- `lab12-remote-breakpoints`: load symbols and set breakpoints by function name and raw address.
- `lab13-registers-and-exceptions`: inspect register and instruction state around controlled exception-style failures.
- `lab14-mini-kernel-debug`: triage early boot hang, wrong entry, and reset-like behavior.

## Required Packages

On Ubuntu:

```bash
sudo apt install qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch make
```

If plain `gdb` works for your target, you can use it with:

```bash
make GDB=gdb gdb
```

`gdb-multiarch` is the safer default for remote target workflows.

## Build Check

To compile-check all Week 4 targets and scenario builds:

```bash
bash scripts/verify-week4.sh
```

The verification script builds the RISC-V ELF targets but does not run interactive QEMU/GDB sessions.

## Two-Terminal Workflow

Most debug sessions use two terminals.

Terminal 1:

```bash
make debug
```

Terminal 2:

```bash
make gdb
```

Inside GDB:

```gdb
target remote :1234
info registers
x/i $pc
continue
```

Some lab `make gdb` targets connect automatically. If the connection fails, check that QEMU is already running with `make debug`.

## Week 4 First-Response Habits

For a paused QEMU target:

```gdb
target remote :1234
info registers
x/i $pc
continue
```

For a missing function breakpoint:

```gdb
info files
info functions
```

For an entry address question:

```bash
riscv64-unknown-elf-nm -n build/kernel.elf
riscv64-unknown-elf-objdump -d build/kernel.elf
```

```gdb
break *0x80000000
```

For a hang:

```gdb
Ctrl-C
info registers
x/i $pc
x/32gx $sp
```

For reset-like behavior:

```bash
make log
```

or:

```bash
qemu-system-riscv64 ... -d int,cpu_reset -D qemu.log
```
