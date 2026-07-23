# Lab 12: Bare-Metal Entry, Symbols, and Raw Breakpoints

## Goal

Compare the startup of a normal C process with a kernel-style bare-metal
target, then use GDB symbols and raw addresses to prove how QEMU reaches the
code you built.

By the end of this lab, you should be able to explain:

- the intended roles of RISC-V M-mode, S-mode, and U-mode;
- why user programs normally run in U-mode, an OS kernel normally runs in
  S-mode, and initial machine startup begins in M-mode;
- why this `-bios none` target begins and remains in M-mode;
- why the lab does not yet perform an M-to-S or S-to-U transition;
- why `.c`, `main`, and `kernel_entry` do not select a privilege mode;
- why bare-metal code needs its own `_start` and stack setup;
- where `0x80000000` comes from and why it is not the reset address;
- how `_start`, `kernel_entry`, and `main` divide startup responsibilities;
- why `0x10000000` is a UART device address rather than a code address;
- how symbolic and raw-address breakpoints provide different evidence.

The main path to remember is:

```text
QEMU reset code (M-mode, near 0x1000)
    -> ELF entry at 0x80000000
    -> _start
    -> set sp
    -> kernel_entry
    -> init_console
    -> main
    -> debug_checkpoint
    -> scheduler_checkpoint
    -> halt_loop
```

## What You Will Verify

During the guided exercise, you will combine static ELF inspection with live
GDB state to verify:

- why the ELF entry and `_start` are both `0x80000000`;
- how execution moves from QEMU's reset code into `_start`;
- when `_start` establishes `sp`;
- how control passes through `kernel_entry`, `init_console`, `main`, and the
  named checkpoints;
- why symbolic and raw-address breakpoints identify the same entry instruction.

## 1. Ordinary Process Startup vs Bare-Metal Startup

### Ordinary Linux C Program

A typical Linux executable is a program file. When you run it, the operating
system creates a process, maps the executable into that process, prepares a
user stack, and begins at the executable's entry point. A C runtime startup
routine, commonly named `_start`, prepares the language environment and calls
your `main` function.

```text
Linux kernel creates a process
    -> program loader maps the ELF
    -> C runtime _start
    -> runtime initialization
    -> main(argc, argv)
```

The resulting process normally executes in U-mode. It is not in U-mode because
the source file ends in `.c` or because the function is named `main`; it is in
U-mode because the operating system created a user process and arranged the
CPU privilege state.

### This Lab's Bare-Metal Program

This target has no running guest operating system, process loader, or hosted C
runtime. The Makefile deliberately uses:

```text
-O0            disable optimization so GDB observations stay straightforward
-ffreestanding  do not assume a hosted C environment
-nostdlib       do not link the normal C library or runtime libraries
-nostartfiles   do not link the platform's normal startup objects
```

`-O0` is a teaching and debugging choice, not a requirement of bare-metal C.
It keeps function boundaries, variable locations, and instruction stepping
easier to relate to the source code.

`build/kernel.elf` is a program image, but it is not an OS-managed process.
QEMU loads it into a virtual machine, and the target supplies its own `_start`.

```text
QEMU reset
    -> our _start
    -> establish sp
    -> call kernel_entry
    -> call main ourselves
```

There is also no normal `exit` service. If `main` and `kernel_entry` return,
`_start` enters `halt_loop` and waits.

## 2. Which RISC-V Mode Is This Code Using?

RISC-V defines three main privilege modes:

```text
M-mode  Machine mode: highest privilege; the CPU starts here
S-mode  Supervisor mode: normally used by an operating-system kernel
U-mode  User mode: normally used by user programs
```

This command is important:

```bash
qemu-system-riscv64 -machine virt -nographic -bios none \
  -kernel build/kernel.elf
```

`-bios none` tells QEMU not to load its normal platform firmware, such as
OpenSBI. Under this lab's default QEMU TCG setup, the virtual CPU resets in
M-mode, executes QEMU's small reset sequence, and jumps to our ELF entry. The
lab never executes `mret` or `sret` to leave that mode, so `_start`,
`kernel_entry`, and `main` all execute in M-mode.

Lab12 is in M-mode.

## 3. Where Does `0x80000000` Come From?

普通 Linux 程序可以依赖系统提供的默认链接布局，因为 OS loader 和 C runtime 对这种布局有共同约定。
Lab 12 是裸机目标，需要明确回答：
```text
代码放到哪个物理地址？
第一条自己的指令在哪里？
栈存储放在哪里？
QEMU 应该跳到哪里？
```

默认 linker script 不知道我们希望 QEMU virt 的代码从 0x80000000 开始，也不一定知道 .text.start 必须排在最前面。
所以裸机程序通常需要自己的 linker script。

`linker.ld` 是link阶段交给linker的一份linker script, 交代了ELF文件的布局。

Makefile 中最终链接命令类似：

```bash
riscv64-unknown-elf-gcc -T linker.ld \
  build/start.o build/main.o -o build/kernel.elf
```

它会调用 RISC-V GNU linker，并把最终ELF文件的布局通过参数 `-T linker.ld` 交给链接器。

```text
Makefile
   │  riscv64-unknown-elf-gcc -T linker.ld ...
   ▼
GCC driver
   │  invokes the RISC-V GNU linker
   ▼
linker
   ├── reads build/start.o
   ├── reads build/main.o
   ├── reads linker.ld
   └── writes build/kernel.elf
                  │
                  ├── QEMU loads the ELF
                  └── GDB reads symbols and debug information
```

各阶段读取的内容不同：

| 阶段 | 读取什么 | 作用 |
|---|---|---|
| 编译器/汇编器 | `main.c`、`start.S` | 生成尚未完成最终布局的 `.o` 文件 |
| 链接器 | `.o` 文件和 `linker.ld` | 合并代码与数据、分配地址并生成 ELF |
| QEMU | `build/kernel.elf` | 根据 ELF program headers 加载目标并取得入口 |
| GDB | `build/kernel.elf` | 读取符号、地址和调试信息，控制 QEMU 中的 CPU |
| CPU | 加载到内存中的机器指令 | 从 PC 指向的位置取指并执行 |

因此 QEMU、GDB 和 CPU 都不直接读取原始 `linker.ld`。它们看到的是链接器
已经写进 `kernel.elf` 的结果。

### Input section 与Output section

编译每个源文件时，编译器会在 `.o` 文件中产生许多输入 section。例如：

```text
build/start.o
  .text.start   _start、halt_loop
  .bss          stack、stack_top

build/main.o
  .text         uart_putc、main、kernel_entry 等函数
  .rodata       字符串常量
  .bss          debug_state、scheduler_state、main_return
  .debug_*      调试信息
```

链接器读取所有输入 section，再按照 `linker.ld` 把它们组合成最终 ELF 的
输出 section：

```text
多个 .o 中的 .text.start/.text.*  ──>  kernel.elf 的 .text
多个 .o 中的 .rodata.*            ──>  kernel.elf 的 .rodata
多个 .o 中的 .data.*              ──>  kernel.elf 的 .data
多个 .o 中的 .bss.*               ──>  kernel.elf 的 .bss
```

这里的输入和输出很重要：`*(.text*)` 不是在调用某个函数，而是在说：

> 从所有输入文件中收集名称匹配 `.text*` 的 section，放进当前输出 section。

第一个 `*` 表示所有输入文件；括号中的 `.text*` 表示 `.text`、
`.text.foo` 等所有以 `.text` 开头的 section。


```text
start.o(.text.start) ─┐
main.o(.text*)       ─┴─> output .text
main.o(.rodata*)     ───> output .rodata
main.o(.data*)       ───> output .data
start.o/main.o(.bss*)───> output .bss
```

### ELF Entry

The linker script also contains:

```ld
ENTRY(_start)
```

This writes `_start`'s resolved address into the ELF header's entry-point
field. `ENTRY(_start)` chooses the entry symbol, but it does not place the
symbol by itself; the section layout does that.

The intended equality is therefore:

```text
start of linked .text
    = address of _start
    = ELF entry point
    = 0x80000000
```

### Reset Address Is Different

For QEMU's RISC-V `virt` machine, the small reset sequence begins in mask ROM
near `0x1000`. It then jumps to the loaded ELF entry. Therefore:

```text
0x00001000 area  QEMU reset code; not part of start.S
0x80000000       first instruction of this lab's own program
```

With QEMU's `-S` option, GDB may initially show a PC near the reset vector. That
is expected. `break *0x80000000` catches the later boundary where the CPU first
enters code from this lab.

## 4. What Is Inside `build/kernel.elf`?

上一节说明了链接器如何把 `_start` 和 ELF entry 安排到 `0x80000000`。
现在再查看这个构建结果本身：ELF 不只是一串连续的机器指令，其中还包括
供 loader 使用的元数据、供 linker 和 debugger 使用的命名 section，以及
需要放入目标内存的内容。

```text
build/kernel.elf on disk
┌──────────────────────────────────────────┐
│ ELF header                               │
│   machine: RISC-V                        │
│   entry: 0x80000000                      |
├──────────────────────────────────────────┤
│ program header table                     │
│   one or more PT_LOAD segments           │______ QEMU uses the
│   file offsets, memory addresses, sizes  │     │ program headers
│   and R/W/X permissions                  │     │ to load memory
├──────────────────────────────────────────┤     │
│ loadable sections                        │     │
│   .text.start  _start from start.o       │     │
│   .text        C functions from main.o   │     │
│   .rodata      UART message strings      │     │
│   .data        nonzero initialized data  │     │
│   .bss         zero-initialized state    │     │
├──────────────────────────────────────────┤     │
│ link/debug information                   │     │ GDB uses symbols
│   .symtab / .strtab  names and addresses │     │ and debug sections
│   .debug_*          source-level details │     │
└──────────────────────────────────────────┘     │
                                                 │
                                                 ▼
QEMU virtual-machine memory after loading

higher addresses
┌──────────────────────────────────────────┐
│ stack_top                                │ <- initial sp
│ 4096-byte stack                          │
│ main_return                              │
│ scheduler_state / debug_state            │
├──────────────── .bss ────────────────────┤
│ nonzero initialized globals, if present  │
├──────────────── .data ───────────────────┤
│ string literals                          │
├──────────────── .rodata ─────────────────┤
│ C functions from main.c                  │
│ halt_loop                                │
│ _start                                   │ <- ELF entry
└──────────────── .text ───────────────────┘
0x80000000

0x10000000 -> UART MMIO; not a kernel.elf section
```


## 5. `_start`, `kernel_entry`, and `main`

### `_start`: Machine-Level Software Entry

`_start` runs before C can assume a usable stack:

```asm
_start:
    la sp, stack_top
    call kernel_entry
```

Its job is intentionally small:

- load a valid stack pointer;
- enter the C side using the RV64 calling convention;
- wait safely if the C side returns.

### `kernel_entry`: Assembly-to-C Boundary

`kernel_entry` is the first C routine called by startup assembly. It calls the
console boundary, calls `main`, and records the return value for GDB.

The name does not change CPU privilege. It describes the software layer at
which hand-written startup code hands control to kernel-style C code.

### `main`: An Ordinary C Function We Call Explicitly

In this lab, `main` is not discovered or invoked by QEMU. `kernel_entry` calls
it like any other C function. We use the familiar name to compare this target
with a hosted C program.

### The ABI Still Matters

Crossing from assembly to C is an ordinary RV64 function call:

- `_start` supplies a 16-byte-aligned stack region;
- `call kernel_entry` puts the return address in `ra`;
- C uses `sp` for its stack frame;
- an integer return value comes back in `a0`.

None of these operations changes privilege mode.

## 6. Why Can This C Code Print Without `printf`?

There is no libc, so the target cannot use normal `printf`. Instead, it talks
directly to QEMU's emulated NS16550A-compatible UART through memory-mapped I/O
(MMIO).

Compare the two addresses:

```text
0x80000000  RAM containing executable instructions such as _start
0x10000000  UART0 device registers on QEMU's virt board
```

The code treats the UART address as a byte pointer:

```c
volatile unsigned char *uart =
    (volatile unsigned char *)0x10000000UL;
```

MMIO means CPU loads and stores are interpreted as accesses to device
registers. `volatile` tells the compiler that every access must really occur;
the device can change independently of ordinary program memory.

Common NS16550A-compatible UART registers form this small address map:

| Offset | Register | Purpose |
|---|---|---|
| `+0` | RBR / THR | Read a received byte / write a byte to transmit |
| `+1` | IER | Enable or disable UART interrupt sources |
| `+2` | IIR / FCR | Inspect interrupt status / control the FIFOs |
| `+3` | LCR | Configure character width, stop bits, and parity |
| `+4` | MCR | Control modem and UART output signals |
| `+5` | LSR | Inspect receive and transmit status |
| `+6` | MSR | Inspect modem input-signal status |
| `+7` | SCR | General scratch register |

The implementation polls UART register 5 until bit `0x20` says the transmit
holding register is ready, then writes one character to register 0. This is
polling, not interrupt-driven I/O.

`init_console` is a named startup boundary and useful breakpoint. It is not a
complete production UART initialization routine. QEMU presents the UART in a
state suitable for this small polling example. UART configuration, buffering,
PLIC routing, and interrupt handlers belong to later ECE391-style work.

## 7. QEMU, GDB, and the Same ELF

`build/kernel.elf` has two related roles:

```text
QEMU reads its loadable segments, addresses, entry point, and machine code.
GDB reads its symbols and debug information.
GDB then controls QEMU's live virtual CPU through the remote protocol.
```

A symbol breakpoint asks GDB to resolve a name:

```gdb
break kernel_entry
```

A raw breakpoint uses an instruction address directly:

```gdb
break *0x80000000
```

The `*` means that the expression is an instruction address. A raw breakpoint
is useful when symbols are missing, stale, or suspected of being loaded at the
wrong address. It does not make a wrong hard-coded address correct.

## Guided Mode

### Step 1: Build the Freestanding Target

```bash
make clean
make
```

The build path is:

```text
start.S -> build/start.o
main.c  -> build/main.o

start.o + main.o + linker.ld -> build/kernel.elf
```

### Step 2: Prove the Static Address Chain

Check the ELF header:

```bash
riscv64-unknown-elf-readelf -h build/kernel.elf
```

`readelf` reads ELF metadata; `-h` selects the ELF header.

Look for:

```text
Entry point address:               0x80000000
```

Check sorted symbols:

```bash
riscv64-unknown-elf-nm -n build/kernel.elf
```

`nm` lists symbols and their resolved addresses; `-n` sorts them numerically
instead of alphabetically.

Look for `_start` at `0x80000000`, followed by symbols such as `halt_loop`,
`kernel_entry`, `main`, `init_console`, and `debug_checkpoint`.

Check instructions:

```bash
riscv64-unknown-elf-objdump -d build/kernel.elf
```

`objdump -d` disassembles executable sections so you can connect symbol
addresses to the instructions the CPU will execute.

Look for the first instructions under `_start`. Confirm that startup establishes
`sp` and calls `kernel_entry`.

Next, inspect the program header table that QEMU uses as its loading plan:

```bash
riscv64-unknown-elf-readelf -l build/kernel.elf
```

Find every `PT_LOAD` entry (shown as `LOAD` by `readelf`) and record:

- its virtual and physical address;
- `FileSiz` and `MemSiz`;
- its `R`, `W`, and `E` permissions.

Then find `Section to Segment mapping` at the end of the output. It shows which
output sections the linker placed in each segment. This is the evidence that
connects the section layout from `linker.ld` to the address ranges QEMU loads;
QEMU follows the `PT_LOAD` entries rather than loading sections by name.

If a writable `PT_LOAD` has `MemSiz` larger than `FileSiz`, the extra runtime
space normally covers a `NOBITS` section such as `.bss`; the loader supplies
that memory without storing the same number of zero bytes in the ELF file.
Also confirm that `.symtab` and `.debug_*` do not appear in a `PT_LOAD`: GDB can
read them from `kernel.elf`, but QEMU does not need to load them into guest
memory.



### Step 3: Start QEMU Paused

In terminal 1:

```bash
qemu-system-riscv64 -machine virt -nographic -bios none \
  -kernel build/kernel.elf -s -S
```

Meaning:

```text
-s  open QEMU's GDB stub on TCP port 1234
-S  hold the virtual CPU at reset until GDB continues it
```

After you understand the full command, `make debug` is the shortcut.

### Step 4: Connect GDB

In terminal 2:

```bash
gdb-multiarch build/kernel.elf
```

Then:

```gdb
set architecture riscv:rv64
target remote :1234
```

After you understand the manual commands, `make gdb` starts GDB, selects RV64,
and connects automatically.

### Step 5: Inspect the Reset State

```gdb
info registers pc sp ra
x/i $pc
```

Questions:

- Is `$pc` already `0x80000000`, or is it still in QEMU's reset code?
- Has this lab's `_start` initialized `sp` yet?

Do not assume; record what your QEMU version shows.

### Step 6: Catch the Raw Entry Address

```gdb
break *0x80000000
info breakpoints
continue
```

Now inspect:

```gdb
info registers pc sp ra
x/i $pc
info symbol $pc
```

Expected evidence:

```text
$pc == 0x80000000
GDB identifies the location as _start
sp has not yet been established by _start
```

The raw address came from the linker and ELF inspection in Step 2; it was not a
magic number supplied by GDB.

### Step 7: Observe the Assembly-to-C Boundary

Set symbolic breakpoints before continuing:

```gdb
break kernel_entry
break init_console
break main
break debug_checkpoint
break scheduler_checkpoint
break halt_loop
info breakpoints
continue
```

At `kernel_entry`:

```gdb
info registers pc sp ra a0
x/i $pc
```

Compare `sp` with its value at `_start`. It should now point into the stack
reserved in `start.S`. `ra` should describe the return path to startup code.

Continue through the named boundaries:

```gdb
continue
info registers pc sp ra
x/i $pc
```

Repeat until you have observed `init_console`, `main`, `debug_checkpoint`,
`scheduler_checkpoint`, and finally `halt_loop`. The stop order is stronger
evidence than UART messages alone.

### Step 8: Inspect C State

At each checkpoint entry, remember that the first source statement has not
necessarily executed yet. Continue until `halt_loop`, where all C routines have
returned, then inspect:

```gdb
p/x debug_state
p/x scheduler_state
p main_return
```

Expected final values after the corresponding code has run:

```text
debug_state     = 0x39141201
scheduler_state = 0x39141202
main_return     = 0
```

### Step 9: Compare Symbolic and Raw Breakpoints

Restart QEMU so the target is paused at reset again. Reconnect GDB, then remove
the old breakpoints:

```gdb
delete
break _start
break *0x80000000
info breakpoints
```

Both breakpoints resolve to the same address in this correctly linked build.
The first depends on the `_start` name; the second independently names the
instruction address.

## Connection to ECE391

ECE391 uses the same RISC-V privilege model and extends this startup foundation:

```text
Lab 12
  M-mode reset -> _start -> kernel_entry -> main

ECE391-style progression
  initial boot
    -> establish an S-mode kernel
    -> load an ELF user program
    -> prepare user registers and stack
    -> sret
    -> run the program in U-mode
    -> trap back to the S-mode kernel on interrupts, exceptions, or syscalls
```

ECE391 also uses MMIO devices such as the UART and later adds interrupt
controllers and trap handlers. Lab 12 intentionally stops before those
mechanisms so you can first prove the entry address, stack setup, C boundary,
and remote-debugging workflow.

One address warning: later kernel designs may also map kernel memory beginning
at `0x80000000`. Always derive an address from the linker script, ELF, memory
map, and current project rather than assuming every RISC-V target uses the same
layout.

## Review Questions

1. Why is a normal Linux C program usually in U-mode?

   Because the operating system creates it as a protected user process. The
   `.c` suffix and the name `main` do not select the privilege mode.

2. Why does this lab execute in M-mode?

   QEMU resets the CPU in M-mode, `-bios none` omits normal platform firmware,
   and the lab never executes a privilege-return instruction to leave M-mode.

3. Why can QEMU not simply call `main` like a C function?

   QEMU loads a machine image and transfers CPU control to its ELF entry. It is
   not a hosted C runtime and does not know or prepare C's required environment.

4. Which code establishes the stack?

   `_start` in `start.S` loads `stack_top` into `sp` before calling C.

5. What three facts make `_start` equal `0x80000000` in this lab?

   The linker location begins at `0x80000000`, `.text.start` is placed first,
   and `ENTRY(_start)` records the resolved `_start` address as the ELF entry.

6. Is `0x80000000` QEMU's RISC-V reset address?

   No. QEMU's `virt` reset code begins near `0x1000` and later jumps to this
   ELF's entry at `0x80000000`.

7. What does the `*` mean in `break *0x80000000`?

   Treat `0x80000000` as an instruction address rather than a function or
   source-line name.

8. Why does GDB need `build/kernel.elf`?

   It contains the architecture metadata, symbol names, addresses, and debug
   information that GDB relates to QEMU's live CPU state.

9. What is `0x10000000` in this lab?

   The MMIO base of QEMU `virt` UART0, not ordinary RAM containing code.

10. Why is the UART pointer `volatile`?

    Device registers can change independently and every requested load or
    store must actually reach the device rather than being optimized away.

11. Does `call kernel_entry` enter kernel mode?

    No. It is an ordinary function call. This lab is already in M-mode, and the
    call does not change that mode.

12. What later mechanism can return from an S-mode kernel to U-mode?

    After preparing the saved status, PC, registers, and user stack, an
    S-mode kernel can use `sret` to restore U-mode execution.
