# GDB Command Cheatsheet by Symptom

Use this as a first-response card. Start with the symptom, then run the first few commands before reading more source code.

## Program Crashed / 程序崩溃

First commands:

```gdb
run
bt
frame 0
info locals
info args
list
```

Purpose: find where the program crashed, which call path led there, and whether the current function received bad data.

## Program Hangs / 程序卡死

First commands:

```text
Ctrl-C
```

```gdb
bt
info registers
x/i $pc
```

Purpose: interrupt the program and inspect where it is stuck.

## Wrong Value / 输出或变量值不对

First commands:

```gdb
break main
run
frame
list
next
print variable_name
display variable_name
info locals
```

Purpose: watch the value change step by step until it first differs from your expectation.

## Where Am I? / 当前运行到哪里了

First commands:

```gdb
frame
list
where
info line
```

Purpose: confirm the current function, source file, line number, nearby source code, and related instruction address range.

Useful Lab 01 examples:

```gdb
p count
p total
p scores[0]
display count
display total
display scores[0]
display *scores@4
display (double)total / count
display/x count
display/i $pc
info locals
```

Managing automatic displays:

```gdb
info display
undisplay 1
disable display 1
enable display 1
```

Purpose: use `display` for values you want printed every time GDB stops; use `p` for one-time inspection.

For array parameters that GDB shows as pointers:

```gdb
p *scores@4
x/4dw scores
```

Purpose: `p *scores@4` prints a C-style artificial array; `x/4dw scores` inspects the same memory as four decimal words.

## Variable Suddenly Changes / 变量突然变坏

First commands:

```gdb
watch variable_name
continue
bt
info locals
```

Purpose: stop at the exact instruction that modifies the watched value.

## Suspicious Pointer / 指针可疑

First commands:

```gdb
print ptr
ptype ptr
x/16xw ptr
```

Purpose: inspect the pointer value, its type, and the memory it points to.

## Array or Nearby Memory Corruption / 数组或附近内存被写坏

First commands:

```gdb
p &array[0]
p &array[index]
p &nearby_field
x/16xw array
bt
info registers
```

Purpose: compare addresses and inspect raw memory to see whether an out-of-bounds array access overlaps nearby data.

## Heap Lifetime Bug / 堆对象生命周期问题

First commands:

```gdb
p ptr
p *ptr
ptype *ptr
x/32xb ptr
```

With AddressSanitizer:

```bash
make asan
```

Purpose: distinguish "the pointer still has an address" from "the heap object is still alive."

## Existing Core File / 已有 core 文件

First commands:

```bash
gdb ./build/program core
```

```gdb
bt
frame 0
info args
info locals
x/i $pc
```

Purpose: reconstruct a crash from saved process state after the program has already exited.

## Stack May Be Corrupted / 栈可能坏了

First commands:

```gdb
bt
info registers
x/32xw $rsp
```

On 32-bit x86:

```gdb
x/32xw $esp
```

Purpose: check whether the stack pointer and return path still look plausible.

## Source View Is Not Enough / 源码视角不够

First commands:

```gdb
disassemble
x/i $pc
si
ni
info registers
```

Purpose: inspect the real instruction stream and CPU state.

On 32-bit x86, prefer the explicit instruction pointer and stack pointer names:

```gdb
x/i $eip
x/16xw $esp
p/x $eax
```

Purpose: connect source-level arguments and return values to machine-level state.

## Build Looks Wrong / 构建结果不对

First commands:

```bash
make -n
make clean
make
make VERBOSE=1
ls -l build/*.o build/program
```

Purpose: confirm what commands Make is actually running and whether the object files or executable are stale.

If a header changed but nothing rebuilt, inspect the relevant Makefile dependency rule.

## Linker or Symbol Problem / 链接或符号问题

First commands:

```bash
nm build/*.o
objdump -t build/*.o
readelf -s build/*.o
nm ./build/program
```

In GDB:

```gdb
info functions
break function_name
```

Purpose: confirm whether a function or symbol exists in object files and in the final binary.

Useful `nm` markers:

```text
U name  uses the symbol but does not define it
T name  defines a text/code symbol
```

## C Calls Assembly and State Changes / C 调用汇编后状态变了

First commands:

```gdb
break assembly_function
disassemble assembly_function
info registers
x/i $eip
stepi
x/16xw $esp
```

Purpose: inspect whether the assembly helper preserved callee-saved registers and returned with a plausible stack.

## QEMU Target Stuck / QEMU 目标卡住

First commands:

```gdb
target remote :1234
info registers
x/i $pc
continue
```

Purpose: connect to the paused or running RISC-V QEMU target and inspect CPU state.

## QEMU Reset or Triple-Fault-Like Behavior / QEMU 重启或疑似三重故障

First commands:

```bash
qemu-system-riscv64 ... -d int,cpu_reset -D qemu.log
```

Then in GDB:

```gdb
target remote :1234
info registers
x/i $pc
```

Purpose: determine whether exception handling failed badly enough to reset the virtual CPU.

## Week 4 QEMU Remote Debugging Add-ons

If QEMU was started with `-S`, it is expected to wait until GDB sends `continue`.

Remote breakpoint by function does not work:

```gdb
info files
info functions
break kernel_entry
```

From the shell:

```bash
riscv64-unknown-elf-nm -n build/kernel.elf
riscv64-unknown-elf-objdump -d build/kernel.elf
```

Purpose: confirm that GDB loaded the symbol-bearing ELF and that the function name exists at the address you expect.

Entry address looks wrong:

```bash
riscv64-unknown-elf-nm -n build/kernel.elf
riscv64-unknown-elf-objdump -d build/kernel.elf
```

```gdb
info files
p/x $pc
x/i $pc
break *0x80000000
```

Purpose: compare the CPU's current instruction pointer with the link address and symbol table.

Stack looks wrong in QEMU:

```gdb
info registers
p/x $sp
x/32gx $sp
bt
```

Purpose: decide whether the target has a plausible RISC-V stack before trusting calls, returns, or backtraces.

In Week 4 lab directories, use:

```bash
make log
```

Purpose: run QEMU with `-d int,cpu_reset -D build/.../qemu.log` for reset-like failures.
