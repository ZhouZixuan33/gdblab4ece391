# Week 3: Makefile, Symbols, RISC-V 32, and Assembly View

Week 3 bridges source-level C debugging to build artifacts, symbols, RV32 calling convention, and mixed C/assembly inspection. The main habit is to ask what layer you are actually debugging: source file, object file, symbol table, disassembly, or assembly helper.

## Labs

- `lab07-makefile-deps`: debug stale builds caused by missing header dependencies.
- `lab08-linker-symbols`: inspect object files and symbols behind linker errors and GDB function lookup.
- `lab09-riscv-calling-convention`: connect RV32 arguments, `a0/a1/a2`, `ra`, `sp`, `s0/fp`, `pc`, and return values.
- `lab10-c-and-asm`: debug a C call into RISC-V assembly and identify a callee-saved register violation.

## Required Packages

Week 3 uses the base tools from Week 1-2 plus the RISC-V cross compiler and binutils:

```bash
sudo apt install build-essential gdb make binutils gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

Lab09 and Lab10 build RV32 object and disassembly artifacts. They do not use QEMU yet. QEMU and GDB remote debugging start in Week 4.

```text
build/*.o
build/*.s
build/*.dump
```

## Build Check

To compile-check the completed Week 3 labs:

```bash
bash scripts/verify-week3.sh
```

The verification script builds the labs but does not run interactive GDB sessions. The GDB command practice lives inside each lab README.

## Week 3 First-Response Habits

For stale builds:

```bash
make -n
ls -l build/*.o build/lab*
make clean
```

For symbol/linker problems:

```bash
nm build/*.o
objdump -t build/*.o
readelf -s build/*.o
```

For RV32 calling-convention inspection:

```bash
riscv64-unknown-elf-objdump -dr build/*.o
riscv64-unknown-elf-nm build/*.o
riscv64-unknown-elf-readelf -s build/*.o
```

For C/assembly boundaries:

```bash
riscv64-unknown-elf-nm build/main.o build/asm_helpers.o
riscv64-unknown-elf-objdump -dr build/asm_helpers.o
less build/lab10.combined.dump
```
