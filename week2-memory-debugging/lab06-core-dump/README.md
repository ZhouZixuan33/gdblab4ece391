# Lab 06: Core Dump Analysis

## Goal

Use `ulimit -c unlimited`, `gdb ./prog core`, `bt`, `info locals`, `info args`, and `x/i $pc` to debug a program after it has already crashed.

## Failure Scenario

Run the program:

```bash
make run
```

You should see a segmentation fault:

```text
Loaded command launch-shell with 3 args.
Segmentation fault (core dumped)
```

The program builds a command with three arguments, but `argv[1]` is `NULL`. Later, `checksum_argument` reads `arg[0]`, which crashes when `arg == NULL`.

The goal is to reconstruct this from the executable and a core file, not by rerunning step by step from the beginning.

## Where This Shows Up / Common Scenarios

Core dump analysis is useful when the failure already happened:

- a test crashed overnight
- a remote VM left behind a core file
- a program died before you could attach GDB
- a kernel-style target reset or crashed after recording state elsewhere

In ECE391-style debugging, this habit maps to post-failure investigation: start from the saved machine state, then walk backward through stack frames and arguments.

## Concept Warmup

This lab uses a post-crash debugging loop:

```text
open executable + core -> inspect crash frame -> inspect callers -> identify the bad argument
```

### Core Dump / core 转储

A core dump is a saved snapshot of a process at the time it crashed. It can include registers, stack memory, heap memory, globals, and enough state for GDB to show where the program stopped.

You normally open it with:

```bash
gdb ./build/lab06 core
```

If the core file is named differently, pass that file name instead.

### Program Counter / 程序计数器

`$pc` means the current program counter: the instruction where execution stopped. On x86_64 this corresponds to `$rip`; on 32-bit x86 it corresponds to `$eip`.

Useful commands:

```gdb
x/i $pc
info registers
bt
```

## Guided Mode

Step 1: Build the crashing program.

```bash
make
```

What to look for / 看什么: `build/lab06` exists and was compiled with `-g -O0`.

Step 2: Enable core dumps for this shell.

```bash
ulimit -c unlimited
```

Meaning / 是什么: allow the shell to write core files when a program crashes.

When to use / 什么时候用: before running a program when you want a post-crash snapshot.

Step 3: Run the program outside GDB.

```bash
./build/lab06
```

What to look for / 看什么: the program crashes with a segmentation fault.

Step 4: Find the core file.

Common names include:

```text
core
core.<pid>
```

Try:

```bash
ls -lh core core.* 2>/dev/null
```

If no file appears, your Ubuntu system may route core dumps through a system service. For this lab, use:

```bash
make gdb-core
```

That fallback asks GDB to run the program once and write `build/lab06.core`.

Step 5: Open the executable and core together.

```bash
gdb ./build/lab06 core
```

Or, if using the fallback:

```bash
gdb ./build/lab06 ./build/lab06.core
```

Meaning / 是什么: load the program's symbols and the saved crash state.

What to look for / 看什么: GDB should show that the program terminated with `SIGSEGV`.

Step 6: Print the backtrace.

```gdb
bt
```

Meaning / 是什么: show the call stack at the time of the crash.

What to look for / 看什么: the top frames should include `checksum_argument`, `checksum_command`, `dispatch_command`, and `main`.

Step 7: Inspect the crash frame.

```gdb
frame 0
info args
info locals
list
x/i $pc
```

What to look for / 看什么: `arg` should be `NULL`, and the current instruction/source line reads from `arg[0]`.

Step 8: Move to the caller.

```gdb
up
info locals
info args
list
```

What to look for / 看什么: in `checksum_command`, the loop index `i` should be `1`, and `cmd->argv[i]` should be `NULL`.

Step 9: Inspect the full command object.

```gdb
p *cmd
p cmd->argv[0]
p cmd->argv[1]
p cmd->argv[2]
```

What to look for / 看什么: `argv[1]` is the bad value.

Step 10: Move upward to find where the bad value was created.

```gdb
up
info args
up
list load_boot_command
```

What to look for / 看什么: `load_boot_command` stores `NULL` in `cmd->argv[1]` but still sets `argc = 3`.

## Hint Mode

1. Generate or locate a core file.
2. Open GDB with both the executable and the core file.
3. Print the backtrace.
4. Inspect frame `#0`.
5. Print the crashing function's arguments.
6. Use `x/i $pc` to see the instruction at the saved program counter.
7. Move to the caller and inspect locals.
8. Find which `argv` slot is `NULL`.

## Review Questions

1. What shell command allows core files to be written?

   Answer:

   ```bash
   ulimit -c unlimited
   ```

2. How do you open a program and a core file in GDB?

   Answer:

   ```bash
   gdb ./build/lab06 core
   ```

   Replace `core` with the actual core file name if needed.

3. What command shows the call stack saved in the core file?

   Answer:

   ```gdb
   bt
   ```

4. How do you inspect the crash frame's arguments?

   Answer:

   ```gdb
   frame 0
   info args
   ```

5. How do you show the instruction at the saved program counter?

   Answer:

   ```gdb
   x/i $pc
   ```

6. Why do you need the executable as well as the core file?

   Answer: the executable provides symbols and debug information, while the core file provides the saved crash state.

7. Which function crashes?

   Answer: `checksum_argument`, because it reads `arg[0]`.

8. Which argument is bad?

   Answer: `arg` is `NULL`, passed from `cmd->argv[1]`.

9. What is the actual bug?

   Answer: `load_boot_command` sets `cmd->argc = 3` but leaves `cmd->argv[1]` as `NULL`, so later code treats a missing argument as a real string.
