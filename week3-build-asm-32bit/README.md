# Week 3: Makefile, 32-bit x86, and Assembly View

Week 3 bridges source-level C debugging to build artifacts, symbols, registers, 32-bit stack layout, and mixed C/assembly debugging. The main habit is to ask what layer you are actually debugging: source file, object file, linked binary, CPU register state, or assembly helper.

## Labs

- `lab07-makefile-deps`: debug stale builds caused by missing header dependencies.
- `lab08-linker-symbols`: inspect object files and symbols behind linker errors and GDB function lookup.
- `lab09-x86-calling-convention`: connect 32-bit x86 arguments, `esp`, `ebp`, `eip`, and `eax`.
- `lab10-c-and-asm`: debug a C call into assembly and identify a callee-saved register violation.

## Required Packages

Week 3 uses the base tools from Week 1-2 plus 32-bit compilation support:

```bash
sudo apt install build-essential gdb make binutils gcc-multilib libc6-dev-i386
```

If `gcc -m32` fails, install the 32-bit packages above before starting Lab 09 or Lab 10.

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

For 32-bit stack/register inspection:

```gdb
disassemble
info registers
x/16xw $esp
x/i $eip
```

For C/assembly boundaries:

```gdb
break assembly_function
stepi
info registers
x/16xw $esp
```
