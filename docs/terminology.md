# Terminology / 术语

This document uses lightweight explanations. Simple terms stay short. Concepts with spatial structure include a small diagram.

## breakpoint / 断点

Meaning / 是什么: a place where GDB pauses program execution.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
break main
info breakpoints
```

Debugging use / 调试时怎么用: stop before a suspicious function or line so you can inspect state.

ECE391 connection / 和 ECE391 的联系: useful before initialization code, system calls, interrupt handlers, or any function that corrupts kernel state.

## watchpoint / 观察点

Meaning / 是什么: a debugger stop condition tied to a value or memory location changing.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
watch state.ready_count
continue
```

Debugging use / 调试时怎么用: find who first changes a value incorrectly.

ECE391 connection / 和 ECE391 的联系: useful when process tables, file descriptors, paging data, or global state are corrupted far from the symptom.

## stack frame / 栈帧

Meaning / 是什么: one function call's execution record, including arguments, local variables, saved registers, and return address.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
bt
frame 1
info locals
info args
```

Debugging use / 调试时怎么用: move between caller and callee to find where bad data entered the call chain.

ECE391 connection / 和 ECE391 的联系: kernel panics, exceptions, interrupts, and system calls often require reasoning about call paths and damaged stacks.

Visual model / 图示:

```text
main()
  calls parse_input()
    calls copy_name()
      crashes here

GDB backtrace:

#0 copy_name()
#1 parse_input()
#2 main()
```

## register / 寄存器

Meaning / 是什么: a small CPU storage slot, such as `eax`, `esp`, `eip`, `rax`, `rsp`, or `rip`.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
print $rip
```

Debugging use / 调试时怎么用: inspect the CPU state when source-level variables are not enough.

ECE391 connection / 和 ECE391 的联系: system calls, interrupts, paging, and context switching all depend on registers.

## rsp and rbp / 栈指针和栈帧基准指针

Meaning / 是什么:

`rsp / stack pointer / 栈指针` points to the current top of the stack.

`rbp / base pointer / 栈帧基准指针` usually points near the base of the current function's stack frame when compiling with `-O0`.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
p/x $rsp
p/x $rbp
x/16xw $rsp
```

Visual model / 图示:

```text
High address
+-------------------------+
| caller's stack frame    |
| return address          |
+-------------------------+
| saved old rbp           | <- rbp often points near here
| local variables         |
| temporary stack data    | <- rsp points around current stack top
+-------------------------+
Low address
```

Debugging use / 调试时怎么用: use `rsp` to inspect raw stack memory and `rbp` to orient yourself inside the current stack frame. If the stack is corrupted, these values or nearby return addresses may look strange.

ECE391 connection / 和 ECE391 的联系: calling conventions, assembly helpers, interrupt stubs, context switches, and QEMU debugging all require understanding where the current stack is and how function frames are arranged.

Note / 注意: in a tiny function, `$rsp` and `$rbp` may be equal or very close. That can be normal when the function has no local buffer or the stack has already been restored near the end of the function.

On x86_64 Linux, the first six integer or pointer arguments usually go in registers, not on the stack:

```text
1st arg -> rdi
2nd arg -> rsi
3rd arg -> rdx
4th arg -> rcx
5th arg -> r8
6th arg -> r9
```

Extra arguments, large structs, saved return addresses, and local stack storage are still stack-related, but simple early arguments may not appear at `$rsp`.

## instruction pointer / 指令指针

Meaning / 是什么: the register holding the address of the current or next instruction. On RISC-V it is `pc`; on x86 it is `eip`; on x86_64 it is `rip`.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
x/i $pc
x/i $eip
x/i $rip
```

Debugging use / 调试时怎么用: find exactly what instruction the CPU is executing when the program crashes or hangs.

ECE391 connection / 和 ECE391 的联系: QEMU/kernel debugging often starts by asking where the CPU is executing now.

## address / 地址

Meaning / 是什么: a location in memory where data or instructions are stored.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
p &variable
p pointer
x/16xw pointer
```

Debugging use / 调试时怎么用: compare addresses to see whether two expressions refer to the same memory, or whether an array access has moved past its valid storage.

ECE391 connection / 和 ECE391 的联系: pointer bugs, stack layout, page tables, buffers, and device memory all require address-level reasoning.

## core dump / core 转储

Meaning / 是什么: a saved snapshot of a process at the moment it crashed.

How it appears in GDB / 在 GDB 里怎么看:

```bash
gdb ./program core
```

```gdb
bt
info registers
x/i $pc
```

Debugging use / 调试时怎么用: inspect the call stack, arguments, locals, registers, and memory after the original process has already exited.

ECE391 connection / 和 ECE391 的联系: post-crash debugging teaches the same habit used for saved fault state, QEMU logs, and crash snapshots.

## symbol / 符号

Meaning / 是什么: a name in the binary, such as a function or global variable name, mapped to an address.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info functions
break main
```

Debugging use / 调试时怎么用: set breakpoints by readable names instead of raw addresses.

ECE391 connection / 和 ECE391 的联系: kernel debugging is much easier when GDB has the right symbol file.

## object file / 目标文件

Meaning / 是什么: an intermediate build product, usually ending in `.o`, produced by compiling a source file before final linking.

How it appears in tools / 在工具里怎么看:

```bash
ls build/*.o
nm build/main.o
objdump -t build/main.o
```

Debugging use / 调试时怎么用: inspect object files when the final executable does not contain the function or global name you expected.

ECE391 connection / 和 ECE391 的联系: kernel builds often combine many C and assembly object files; one missing object can remove a handler or entry point from the final binary.

## dependency / 依赖

Meaning / 是什么: an input file that a Make target depends on.

How it appears in Make / 在 Make 里怎么看:

```makefile
build/main.o: main.c config.h
```

Debugging use / 调试时怎么用: if a header changes but an object file does not rebuild, the dependency rule may be incomplete.

ECE391 connection / 和 ECE391 的联系: stale object files can make kernel behavior disagree with the source you are reading.

## linker / 链接器

Meaning / 是什么: the tool that combines object files and libraries into the final executable.

How it appears in errors / 在错误里怎么看:

```text
undefined reference to `function_name'
```

Debugging use / 调试时怎么用: use linker errors and symbol tools to identify which object file uses a symbol and which object file should define it.

ECE391 connection / 和 ECE391 的联系: C files, assembly stubs, and linker scripts all have to agree on names and addresses.

## calling convention / 调用约定

Meaning / 是什么: the rule for passing arguments, returning values, preserving registers, and cleaning up the stack around a function call.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
x/16xw $esp
disassemble
```

Debugging use / 调试时怎么用: use it when a function receives wrong arguments, returns a strange value, or assembly code corrupts state after returning.

ECE391 connection / 和 ECE391 的联系: system calls, interrupt stubs, context switches, and assembly helpers all depend on calling convention discipline.

## callee-saved register / 被调用者保存寄存器

Meaning / 是什么: a register that a function must restore before returning if it changes the register.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
stepi
```

Debugging use / 调试时怎么用: compare register values before and after a C call into assembly.

ECE391 connection / 和 ECE391 的联系: register-save mistakes can make unrelated C code fail after an assembly helper returns.

## caller-saved register / 调用者保存寄存器

Meaning / 是什么: a register that a caller must save itself if it needs the value after a function call.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
disassemble
```

Debugging use / 调试时怎么用: do not assume every register survives a call. Check the calling convention first.

ECE391 connection / 和 ECE391 的联系: low-level code must know which registers can be clobbered across helper calls.

## page fault / 缺页异常

Meaning / 是什么: the CPU tried to access a virtual address that is unmapped or not allowed by page permissions.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
x/i $eip
x/32xw $esp
```

Debugging use / 调试时怎么用: inspect the faulting instruction and the address it tried to use.

ECE391 connection / 和 ECE391 的联系: paging, user pointer checks, NULL access, and permission bits can all lead to page faults.

## general protection fault / 通用保护异常

Meaning / 是什么: the CPU detected a protection-rule violation that is not simply a page translation failure.

How it appears in GDB / 在 GDB 里怎么看:

```gdb
info registers
x/i $eip
disassemble
```

Debugging use / 调试时怎么用: distinguish invalid privilege, descriptor, or control-structure mistakes from ordinary pointer bugs.

ECE391 connection / 和 ECE391 的联系: GDT, IDT, TSS, privilege levels, interrupts, and system calls can trigger this class of problem.

## triple fault / 三重故障

Meaning / 是什么: exception handling failed so badly that the CPU resets.

How it appears in QEMU / 在 QEMU 里怎么看:

```bash
qemu-system-riscv64 ... -d int,cpu_reset -D qemu.log
```

Debugging use / 调试时怎么用: suspect it when QEMU instantly reboots, goes black, or jumps back to startup code.

ECE391 connection / 和 ECE391 的联系: early kernel bugs in IDT, paging, stacks, or exception handlers may leave the CPU unable to report the original error.

## QEMU / QEMU

Meaning / 是什么: a machine emulator used in Week 4 to run a tiny RISC-V `virt` target outside a normal Linux process.

How it appears in commands / 在命令里怎么看?

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf
```

Debugging use / 调试时怎么用? run a kernel-style target in a controlled virtual machine.

ECE391 connection / 和 ECE391 的联系: many early-kernel failures are easier and safer to reproduce in QEMU than on real hardware.

## remote target / 远程调试目标

Meaning / 是什么: the CPU or program controlled by GDB through a remote connection instead of a local `run` command.

How it appears in GDB / 在 GDB 里怎么看?

```gdb
target remote :1234
```

Debugging use / 调试时怎么用? attach GDB to the virtual CPU exposed by QEMU.

ECE391 connection / 和 ECE391 的联系: kernel code is not a normal user-space process, so GDB often controls it through QEMU's remote stub.

## symbol file / 符号文件

Meaning / 是什么: an ELF file that tells GDB the names and addresses of functions and global data.

How it appears in GDB / 在 GDB 里怎么看?

```gdb
symbol-file build/kernel.elf
info functions
```

Debugging use / 调试时怎么用? load readable names such as `kernel_entry` so you can set symbolic breakpoints.

ECE391 connection / 和 ECE391 的联系: if GDB uses the wrong symbol file, breakpoints and source locations can point to the wrong code.

## entry point / 入口点

Meaning / 是什么: the first target instruction or function that should run after QEMU loads the target or after setup code jumps into the kernel.

How it appears in tools / 在工具里怎么看?

```bash
nm -n build/kernel.elf
```

```gdb
break kernel_entry
break *0x80000000
```

Debugging use / 调试时怎么用? prove whether the CPU reached the code that was supposed to start execution.

ECE391 connection / 和 ECE391 的联系: wrong entry addresses can make a kernel appear dead before any useful output exists.

## link address / 链接地址

Meaning / 是什么: the address where the linker assumes code and data will live when the target runs.

How it appears in files / 在文件里怎么看?

```ld
. = 0x80000000;
```

Debugging use / 调试时怎么用? compare GDB's symbol addresses with the CPU's `pc`.

ECE391 connection / 和 ECE391 的联系: a mismatch between load address, link address, and symbol file can make breakpoints miss or disassembly look unrelated.

## boot image / 启动镜像

Meaning / 是什么: the raw disk, kernel image, or ELF target that QEMU loads to start a virtual machine.

How it appears in commands / 在命令里怎么看?

```bash
qemu-system-riscv64 -machine virt -nographic -bios none -kernel build/kernel.elf
```

Debugging use / 调试时怎么用? QEMU runs the target artifact, while GDB uses ELF symbols to map addresses to names.

ECE391 connection / 和 ECE391 的联系: kernel debugging often involves more than one artifact: bootable image, ELF with symbols, object files, and logs.

## QEMU log / QEMU 日志

Meaning / 是什么: a file QEMU writes with CPU, interrupt, exception, or reset details.

How it appears in commands / 在命令里怎么看?

```bash
qemu-system-riscv64 ... -d int,cpu_reset -D qemu.log
```

Debugging use / 调试时怎么用? inspect reset-like behavior when the target fails too quickly for interactive debugging.

ECE391 connection / 和 ECE391 的联系: early exception handling bugs may reset the virtual CPU before source-level debugging shows the original fault.
