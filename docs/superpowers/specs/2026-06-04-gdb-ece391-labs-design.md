# GDB ECE391 Labs Design

Date: 2026-06-04

## Purpose

This project will create a 3-4 week, project-oriented debugging training repo for preparing for UIUC ECE391. The learner already knows enough C to write small programs, but is not yet comfortable using GDB, Makefile-driven builds, 32-bit x86 concepts, assembly-level debugging, QEMU, or GDB remote debugging.

The project should teach debugging as a repeatable workflow. Each lab will start from a concrete failure scenario, then guide the learner through the commands, terminology, mental models, and ECE391 relevance needed to diagnose it.

## Target Environment

- Primary environment: AWS Ubuntu VM.
- OS: Ubuntu 24.04.4 LTS, x86_64.
- Workflow: SSH terminal, Makefiles, GCC, GDB, binutils, QEMU.
- Later labs may use 32-bit x86 compilation and `qemu-system-i386`.

This environment is close enough to ECE391-style Linux command-line work while remaining easy to reset and extend.

## Learning Principles

The project will follow these principles:

- Teach through failure scenarios, not isolated command memorization.
- Explain the practical debugging situations where each command or workflow becomes useful.
- Give beginners a clear starting path so they do not freeze at a crash prompt.
- Gradually remove support through guided walkthroughs, hint-only practice, and review questions.
- Explain GDB terminology in Chinese and English.
- Use diagrams for spatial concepts such as stack frames, call stacks, build dependencies, memory layout, and QEMU-GDB remote connections.

## Practice Modes

Each lab will provide a guided path and a lighter hint path. Review questions check whether the learner can recall the workflow. Challenge-style prompts are optional and should be used only when they add useful review value.

### Guided Mode

Guided Mode is a narrated debugging walkthrough. It gives exact commands and explains each step.

Core debugging steps should include:

- `Command`: the exact shell, Make, GDB, or QEMU command.
- `Meaning / 是什么`: what the command means in simple language.
- `When to Use / 什么时候用`: the debugging situation where this command should come to mind.
- `What to Look For / 看什么`: the important part of the output.
- `Why This Helps / 为什么有用`: why this command narrows the debugging search.
- `Common Scenario / 常见场景`: a concrete situation where the learner would reach for this command.
- `Memory Hook / 记忆钩子`: an optional short cue that helps the learner remember the command.

Repeated setup steps such as `make`, `gdb ./build/labXX`, or `break main` do not need the full block in every lab. They can be shortened once previous labs have already taught them. Spend the detailed explanation budget on the new debugging move introduced by the current lab.

Example format:

```text
Step: Inspect the current function arguments / 查看当前函数参数

Command:
(gdb) info args

Meaning / 是什么:
Show the values passed into the current function.

When to Use / 什么时候用:
Use this after `bt` and `frame` when a function crashes and you need to know whether the caller passed bad data.

What to Look For / 看什么:
Look for NULL pointers, impossible sizes, strange addresses, or values that contradict your expectations.

Why This Helps / 为什么有用:
It tells you whether the current function created the bad value or merely received it from its caller.

Common Scenario / 常见场景:
Use this when a helper crashes and you need to check whether the caller passed a bad pointer, size, flag, or object reference.
```

### Hint Mode

Hint Mode gives the debugging direction but not the exact command list. It helps the learner recall commands from memory.

Example:

```text
1. Stop at `main`.
2. Run until the crash.
3. Inspect the call stack.
4. Move to the caller frame.
5. Inspect the arguments passed into the crashing function.
```

### Optional Challenge Mode

Challenge Mode gives only the failure symptom. It simulates real debugging, but it is optional. Use it for review labs or advanced practice, not as required scaffolding for every lab.

Example:

```text
The program sometimes segfaults when copying a string. Find the first function that receives a bad pointer and explain where the bad value came from.
```

## First-Response Debugging Recipes

First-response recipes should live in `docs/command-cheatsheet.md`, organized by symptom. Do not generate a standard "First 60 Seconds" section in every lab.

Reason: for these beginner labs, a short command-only section often duplicates Guided Mode without enough explanation. The lab document should move from concept warmup into Guided Mode, while the cheatsheet provides quick command lookup once the learner has already practiced the workflow.

A lab may include a short quick-path section only when it adds real value beyond Guided Mode, such as a review lab, a multi-scenario lab where the learner needs to choose one path, or a troubleshooting note for environment-specific setup. Otherwise, skip it.

For a crash:

```gdb
run
bt
frame 0
info locals
info args
list
```

For a hang:

```text
Run the program, interrupt it with Ctrl-C, then inspect:
```

```gdb
bt
info registers
x/i $pc
```

For QEMU remote debugging:

```gdb
target remote :1234
info registers
x/10i $eip
bt
```

## Terminology System

New terminology will be explained in Chinese and English. Each lab will include a `Terminology Capsule` when it introduces terms.

Required format:

```text
Term: stack frame / 栈帧

Meaning / 是什么:
One function call's execution record, including arguments, local variables, saved registers, and the return address.

How it appears in GDB / 在 GDB 里怎么看:
bt
frame 1
info locals
info args

Debugging use / 调试时怎么用:
Use stack frames to understand which function is currently running, who called it, and where bad arguments or corrupted return paths came from.

Common Scenario / 常见场景:
Use this when the crash happens inside a helper or library call, but you need to find the caller that introduced the bad value.
```

Terminology explanations should stay lightweight. Simple terms may use only two or three concise lines. Complex spatial or workflow concepts may include an optional `Visual model / 图示` section using ASCII, Mermaid, or the visual companion.

Core terms to cover include:

- `breakpoint / 断点`
- `watchpoint / 观察点`
- `stack / 栈`
- `stack frame / 栈帧`
- `backtrace / 调用栈回溯`
- `register / 寄存器`
- `instruction pointer / 指令指针`
- `address / 地址`
- `symbol / 符号`
- `object file / 目标文件`
- `linker / 链接器`
- `calling convention / 调用约定`
- `core dump / core 转储`
- `remote target / 远程调试目标`
- `CPU exception / CPU 异常`
- `page fault / 缺页异常`
- `general protection fault / 通用保护异常`
- `double fault / 双重故障`
- `triple fault / 三重故障`

The Week 4 terminology should explain these exception terms as ECE391-relevant debugging concepts:

- `page fault / 缺页异常`: address translation or page permission failed.
- `general protection fault / 通用保护异常`: an x86 protection rule was violated, often involving privilege, segment, descriptor, or control-structure mistakes.
- `double fault / 双重故障`: the CPU hit another exception while trying to handle an earlier exception.
- `triple fault / 三重故障`: exception handling failed so badly that the CPU resets; in QEMU this may look like an instant reboot, black screen, or jump back to startup code.

These terms should be introduced carefully. The labs should teach how to inspect the failure site first, not require the learner to implement a full exception subsystem before understanding the debugging workflow.

## Visual Companion and Diagrams

The project should use diagrams when a concept is easier to understand spatially than verbally.

Persistent documentation should use ASCII diagrams or Mermaid diagrams so it works inside Markdown and terminal workflows.

The visual companion can be used during explanation or review for concepts where an interactive or clearer visual helps. Good candidates:

- Stack frame layout.
- Backtrace as a call-chain map.
- Makefile dependency graph.
- C-to-object-to-link build pipeline.
- QEMU and GDB remote-debug connection.
- Instruction pointer moving through assembly.
- Memory layout for stack, heap, globals, and code.

The visual companion is optional per concept. It should be used when it clarifies the idea, not as a required part of every lab.

Example ASCII diagram for stack frames:

```text
High Address
+----------------------+
| caller stack frame   |  <- main()
| return address       |
| saved ebp/rbp        |
+----------------------+
| current stack frame  |  <- buggy_func()
| local variables      |
| function arguments   |
| saved registers      |
+----------------------+
Low Address
```

Example backtrace model:

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

## Lab Document Template

Each lab should use this structure:

```text
Goal
Failure Scenario(s)
Where This Shows Up / Common Scenarios
Concept Warmup
Guided Mode
Hint Mode
Review Questions
```

Optional sections can be added when they genuinely help:

- `Visual Mental Model`: for spatial concepts such as stack frames, memory layout, or QEMU remote connections.
- `Terminology Capsule`: for dense terminology that would interrupt the main walkthrough.
- `Quick Path`: for review or troubleshooting labs only, when a compact command path adds value beyond Guided Mode.
- `Challenge Mode`: for later review labs where the learner should debug with minimal scaffolding.
- `Reflection`: for open-ended transfer questions after the concrete debugging task is complete.

Expected findings should usually be folded into the failure scenario or guided conclusion. This keeps the lab focused on what the learner is trying to prove, instead of adding a separate answer-key section.

## Repository Structure

```text
gdb-ece391-labs/
  README.md
  setup/
    ubuntu-24.04.md
    packages.md
  week1-gdb-basics/
    lab01-breakpoints/
    lab02-stack-backtrace/
    lab03-watchpoints/
  week2-memory-debugging/
    lab04-array-overflow/
    lab05-heap-lifetime/
    lab06-core-dump/
  week3-build-asm-32bit/
    lab07-makefile-deps/
    lab08-linker-symbols/
    lab09-x86-calling-convention/
    lab10-c-and-asm/
  week4-qemu-remote-gdb/
    lab11-qemu-hello/
    lab12-remote-breakpoints/
    lab13-registers-and-exceptions/
    lab14-mini-kernel-debug/
  docs/
    command-cheatsheet.md
    terminology.md
    ece391-debugging-map.md
```

## Four-Week Curriculum

### Week 1: GDB User-Space Basics

Goal: Build basic GDB muscle memory in ordinary C programs before adding systems complexity.

Lab 01: Hello GDB, breakpoints, and stepping

- Failure scenario: The program produces the wrong output but does not crash.
- Commands: `gdb`, `break`, `run`, `next`, `step`, `continue`, `print`, `display`, `list`.
- Use when: output or state is wrong, but the program still runs, so you need to stop early and inspect values before the symptom is printed.

Lab 02: Backtrace and stack frames

- Failure scenario: A multi-function program segfaults several calls below the original mistake.
- Commands: `bt`, `frame`, `up`, `down`, `info args`, `info locals`.
- Use when: the crash happens inside a helper or library function, and you need to walk back to the caller that introduced the bad pointer or argument.

Lab 03: Watchpoints and changing state

- Failure scenario: A variable or struct field is unexpectedly changed.
- Commands: `watch`, `rwatch`, `awatch`, `condition`, `ignore`, `commands`.
- Use when: a variable, struct field, pointer target, or array element becomes wrong, and you need GDB to stop at the exact write or access.

### Week 2: Memory, Stack, Pointers, and Core Dumps

Goal: Learn how to inspect memory and reason about pointer and lifetime bugs.

Lab 04: Array overflow

- Failure scenario: An out-of-bounds write corrupts adjacent data and may not crash immediately.
- Commands: `x/16xw addr`, `print &var`, `info registers`, `bt`.
- Use when: nearby variables or table entries change unexpectedly after an array or buffer operation.

Lab 05: Heap lifetime

- Failure scenario: Use-after-free or double free.
- Commands: `print ptr`, `x/32xb ptr`, `ptype`, `watch *ptr`, plus AddressSanitizer as a comparison tool.
- Use when: a pointer still has an address, but the object it points to may no longer be valid or may have been freed twice.

Lab 06: Core dump analysis

- Failure scenario: A program has already crashed and only the executable plus core file remain.
- Commands: `ulimit -c unlimited`, `gdb ./prog core`, `bt`, `info locals`, `x/i $pc`.
- Use when: the failure already happened and you need to reconstruct the crash from saved process state.

### Week 3: Makefile, 32-bit x86, and Assembly View

Goal: Bridge from source-level C debugging to build products, symbols, registers, calling conventions, and assembly-level reasoning.

Lab 07: Makefile dependencies

- Failure scenario: A header changes but the expected object file is not rebuilt, or a stale build hides the real source.
- Commands: `make`, `make clean`, `make -n`, `make VERBOSE=1`.
- Use when: the binary behavior does not match the source you edited, or you suspect the wrong object files were rebuilt.

Lab 08: Linker symbols

- Failure scenario: Link failures, missing symbols, or mismatched function declarations and definitions.
- Commands: `nm`, `objdump -t`, `readelf -s`, GDB `info functions`.
- Use when: a breakpoint by function name fails, a symbol is missing, or the linker reports unresolved references.

Lab 09: x86 calling convention

- Failure scenario: Misunderstanding how arguments, return values, saved registers, and stack cleanup work.
- Commands: `disassemble`, `info registers`, `x/16xw $esp`, `si`, `ni`.
- Use when: source-level variables are not enough and you need to see how arguments, return values, and saved registers move through the stack.

Lab 10: C and assembly mixed debugging

- Failure scenario: An assembly helper fails to preserve a register or returns with the stack in the wrong state.
- Commands: `layout asm`, `break *addr`, `x/10i $eip`, `info registers`, `stepi`.
- Use when: execution crosses between C and assembly and the bug appears as a bad register value, bad return, or stack mismatch.

### Week 4: QEMU and GDB Remote Debugging

Goal: Practice the core workflow for debugging a target running under QEMU.

Lab 11: QEMU hello target

- Failure scenario: A target starts paused in QEMU and waits for GDB.
- Commands: `qemu-system-i386 -s -S ...`, `target remote :1234`, `continue`.
- Use when: the program is running in QEMU rather than as a normal local process, so GDB must attach to a remote target.

Lab 12: Remote breakpoints

- Failure scenario: A target runs in QEMU, and the learner must stop at a function or raw address.
- Commands: `symbol-file`, `break function`, `break *addr`, `info breakpoints`.
- Use when: there is no useful output yet and you need to stop at a known function, symbol, or instruction address.

Lab 13: Registers and exceptions

- Failure scenario: Controlled CPU-exception-style failures are triggered, such as invalid memory access, invalid instruction or protection-rule violation, and corrupted control flow.
- Commands: `info registers`, `x/i $eip`, `x/32xw $esp`, `disassemble`.
- Use when: execution stops at an unexpected instruction or address and you need register, instruction, and stack state before source code makes sense.
- Required scenarios:
  - Page fault style: access a clearly invalid or unmapped address and identify the faulting instruction.
  - GPF or invalid-protection style: trigger an invalid instruction or protection-like failure and distinguish it from an ordinary pointer bug.
  - Bad jump: corrupt a function pointer or return path so execution reaches an unexpected address.

Lab 14: Mini kernel debug

- Failure scenario: A tiny kernel or debug target hangs, jumps to the wrong place, fails during early startup, or shows reset/triple-fault-like behavior.
- Commands: `target remote`, `bt`, `info registers`, `x`, `disassemble`, `break *addr`, and optional QEMU logging such as `-d int,cpu_reset -D qemu.log`.
- Use when: the target does not come up cleanly, and you must combine breakpoints, registers, memory inspection, disassembly, and QEMU logs to find where progress stopped.
- Required scenarios:
  - Early boot hang: the target stops making visible progress.
  - Wrong entry address: execution starts or jumps somewhere unexpected.
  - Reset or triple-fault-like behavior: QEMU restarts or returns to startup code before useful output appears.

## Supporting Docs

The repo should include these supporting docs:

- `docs/command-cheatsheet.md`: command lookup organized by symptom.
- `docs/terminology.md`: Chinese-English terminology reference.
- `docs/ece391-debugging-map.md`: map from ECE391-style failure symptoms to debugging commands and labs.
- `setup/ubuntu-24.04.md`: setup steps for Ubuntu 24.04.
- `setup/packages.md`: packages and toolchain notes, including 32-bit and QEMU dependencies.

## Command Lookup by Symptom

The command cheatsheet should be organized around symptoms, not alphabetic command order.

Example:

```text
Program crashed / 程序崩溃:
  run, bt, frame, info locals, info args

Variable becomes wrong / 变量何时变坏:
  watch, rwatch, awatch, condition

Pointer looks suspicious / 指针可疑:
  print ptr, ptype ptr, x/16xw ptr

Stack may be corrupted / 栈可能坏了:
  bt, info registers, x/32xw $rsp or $esp

Program hangs / 程序卡死:
  Ctrl-C, bt, info threads, x/i $pc

Source view is not enough / 源码视角不够:
  disassemble, layout asm, si, ni

QEMU target is stuck / QEMU 目标卡住:
  target remote :1234, info registers, x/i $eip
```

## Scope Boundaries

This design prepares a learning repo. It does not attempt to recreate the full ECE391 codebase or assignments.

The QEMU labs should stay lightweight and educational. They may include a tiny kernel-style debug target, but should avoid becoming a full operating system project. Bootloader, paging, and interrupt concepts can appear as introductory shadows where useful, but the main focus remains debugging workflow.

## Success Criteria

The project succeeds if the learner can:

- Start GDB and use breakpoints, stepping, printing, backtraces, frames, locals, and args without freezing.
- Diagnose common C failure scenarios using a repeatable first-response workflow.
- Inspect memory, addresses, registers, and disassembly with enough confidence to reason about low-level bugs.
- Understand Makefile dependencies, object files, symbols, and link failures well enough to avoid stale-build confusion.
- Connect GDB to a QEMU target and inspect execution state with remote debugging commands.
- Explain core GDB and systems terminology in both English and Chinese.
- Transfer each lab's debugging habit to likely ECE391 failure modes.
