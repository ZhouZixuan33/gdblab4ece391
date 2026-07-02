# Week 3 Build, Symbols, 32-bit x86, and Assembly Debugging Design

Date: 2026-06-29

## Purpose

Week 3 will bridge the learner from source-level C debugging into the build products and machine-level state that matter for ECE391-style work. Week 1 taught basic GDB control flow and stack navigation. Week 2 taught memory inspection, pointer lifetime, and core dump analysis. Week 3 now teaches the next layer down: Makefile dependencies, object files, symbol tables, 32-bit x86 calling conventions, registers, stack layout, and mixed C/assembly debugging.

The week should stay beginner-friendly. Each lab will use a small failure scenario, a guided debugging walkthrough, a hint-only path, and review questions. The labs should not become a kernel project yet; QEMU and remote debugging remain Week 4's responsibility.

## Target Learner

The learner can already:

- Build and run small C programs with `make`.
- Start GDB, set breakpoints, step, print variables, inspect frames, and use `bt`.
- Inspect memory with `x`, reason about pointers, and open a core file.

The learner is not yet comfortable with:

- Diagnosing stale binaries caused by incomplete Makefile dependencies.
- Reading object files and symbol tables.
- Connecting a C function call to 32-bit x86 stack/register behavior.
- Debugging a program that crosses between C and hand-written assembly.

## Design Principles

Week 3 will follow the existing repository style:

- One focused directory per lab.
- `README.md` as the primary teaching artifact.
- A tiny C or C/assembly program that exhibits a concrete symptom.
- A `Makefile` with `all`, `run`, and `clean` targets.
- Build verification through `scripts/verify-week3.sh`.
- Guided Mode, Hint Mode, and Review Questions in every lab.
- Chinese-English terminology where new low-level concepts appear.

The labs should teach "what artifact am I actually debugging?" before teaching more assembly. A learner who does not know whether the executable, object file, or symbol table is current will have a hard time debugging registers.

## Week 3 Lab Sequence

### Lab 07: Makefile Dependencies and Stale Builds

Goal: Teach the learner to suspect stale build products when program behavior does not match source changes.

Failure scenario: The program reads a value from `config.h`, but the Makefile only rebuilds `main.o` when `main.c` changes. After the learner edits `config.h`, `make run` still shows the old behavior because the object file was not rebuilt.

Planned files:

```text
week3-build-asm-32bit/lab07-makefile-deps/
  README.md
  Makefile
  main.c
  config.h
```

Program shape:

- `config.h` defines a visible constant such as `MAX_TASKS`, `SHELL_PROMPT_LIMIT`, or `SCHEDULER_QUANTUM`.
- `main.c` prints the configured value and an expected behavior line.
- The initial Makefile intentionally omits `config.h` from the object dependency rule.

Core commands:

```bash
make
make run
make -n
make clean
ls -l build/main.o build/lab07
```

Guided learning path:

1. Build and run the program.
2. Edit or compare `config.h` with the observed output.
3. Run `make -n` to see that Make does not plan to rebuild the object file.
4. Use `make clean` as a temporary reset.
5. Explain the real fix: add `config.h` to the dependency rule.
6. Rebuild and confirm that the output now matches the header.

Terminology:

- `target / 目标`
- `dependency / 依赖`
- `object file / 目标文件`
- `stale build / 过期构建产物`

ECE391 connection: If a kernel header, syscall number, struct layout, or constant changes but the right object file is not rebuilt, GDB may faithfully debug an old binary. The first bug is then in the build graph, not in GDB.

### Lab 08: Linker Symbols and Function Lookup

Goal: Teach the learner to connect source declarations, object files, linker errors, and GDB symbol lookup.

Failure scenario: `main.c` calls a function declared in a header, but the build either omits the object file that defines it or uses a mismatched implementation name. The learner sees an `undefined reference` or cannot set a function breakpoint until they inspect symbols.

Planned files:

```text
week3-build-asm-32bit/lab08-linker-symbols/
  README.md
  Makefile
  main.c
  handlers.c
  handlers.h
```

Program shape:

- `main.c` calls a small handler-installation function such as `install_keyboard_handler()`.
- `handlers.h` declares the expected function.
- `handlers.c` defines the function for the fixed build.
- The Makefile provides a normal `all` target and a deliberate broken target, such as `make broken-missing-object`, so the lab can demonstrate a linker failure without leaving the default build permanently broken.

Core commands:

```bash
make broken-missing-object
make
nm build/*.o
objdump -t build/*.o
readelf -s build/*.o
```

GDB commands:

```gdb
info functions install
break install_keyboard_handler
run
```

Guided learning path:

1. Trigger the broken build target and read the linker error.
2. Identify the missing symbol name in the error.
3. Build object files and inspect their symbol tables with `nm`.
4. Distinguish defined symbols from undefined references.
5. Build the fixed executable.
6. Start GDB and confirm that `info functions` and `break function_name` can find the symbol.

Terminology:

- `symbol / 符号`
- `undefined reference / 未定义引用`
- `linker / 链接器`
- `symbol table / 符号表`

ECE391 connection: Many ECE391 failures involve names crossing file boundaries: syscall handlers, interrupt stubs, assembly entry points, and C helper functions. Symbol tools help answer whether a name exists in the object files and final executable.

### Lab 09: 32-bit x86 Calling Convention

Goal: Teach the learner to read a simple 32-bit x86 C function call through stack arguments, `esp`, `ebp`, and `eax`.

Failure scenario: A small function such as `sum_triple(a, b, c)` returns a wrong value because one argument is passed incorrectly or one index/order assumption is wrong. The learner must stop inside the callee and compare the C-level argument view with the 32-bit stack/register view.

Planned files:

```text
week3-build-asm-32bit/lab09-x86-calling-convention/
  README.md
  Makefile
  main.c
```

Program shape:

- `main.c` calls a few small arithmetic functions.
- One call intentionally passes arguments in a misleading order or uses the wrong value, producing an output mismatch.
- The function body stays simple so the assembly is readable at `-O0`.
- The Makefile compiles with `-m32 -g -O0 -Wall -Wextra -std=c11 -fno-omit-frame-pointer -fno-pie` and links with `-no-pie` so the frame layout and disassembly stay readable for beginners.

Core GDB commands:

```gdb
break sum_triple
run
info args
disassemble
info registers
x/16xw $esp
si
ni
finish
p/x $eax
```

Guided learning path:

1. Run the program and observe the wrong result.
2. Stop inside the callee.
3. Use `info args` to get the source-level view.
4. Use `info registers` to find `esp`, `ebp`, `eip`, and `eax`.
5. Inspect raw stack words around `$esp` or `$ebp`.
6. Disassemble the function and step a few instructions.
7. Connect the return value to `eax`.
8. Explain how caller-pushed arguments and callee frame setup appear in GDB.

Terminology:

- `calling convention / 调用约定`
- `stack pointer / 栈指针`
- `base pointer / 栈帧基准指针`
- `instruction pointer / 指令指针`
- `return value register / 返回值寄存器`

ECE391 connection: ECE391 uses 32-bit x86 concepts heavily. When source-level C does not explain a failure, the learner must be able to inspect `esp`, `ebp`, `eip`, stack words, and return values.

32-bit setup note: The README and setup docs must clearly tell Ubuntu learners to install 32-bit build support, such as `gcc-multilib` and `libc6-dev-i386`. If the environment lacks these packages, the Makefile should fail with a readable note rather than a mysterious compiler error whenever practical.

### Lab 10: Mixed C and Assembly Debugging

Goal: Teach the learner to debug across the C/assembly boundary and recognize a calling convention violation.

Failure scenario: C calls a hand-written 32-bit assembly checker, which calls a broken helper. The helper modifies a callee-saved register such as `ebx` without restoring it. The checker returns a failure code to C after detecting that `ebx` changed. This is safer and more teachable than deliberately corrupting `esp` for the first mixed C/assembly lab.

Planned files:

```text
week3-build-asm-32bit/lab10-c-and-asm/
  README.md
  Makefile
  main.c
  asm_helpers.S
```

Program shape:

- `main.c` calls an assembly checker such as `check_preserves_ebx()` and reports that the helper violated the calling convention.
- `asm_helpers.S` contains a checker that sets `ebx` to a sentinel value, calls a deliberately broken helper, then compares `ebx` after the call.
- The broken helper clobbers `ebx` without saving and restoring it.
- The lab should also show the expected fix conceptually: save and restore `ebx` with `push`/`pop` or avoid using a callee-saved register.
- The default program should remain stable enough to run repeatedly; it should not rely on random crashes.
- The Makefile should use `-m32`, `-fno-pie`, and `-no-pie` so `ebx` is not reserved for position-independent executable support during the lab.

Core GDB commands:

```gdb
break main
break asm_helper
run
disassemble asm_helper
info registers
x/10i $eip
stepi
nexti
x/16xw $esp
```

Optional TUI command:

```gdb
layout asm
```

Guided learning path:

1. Build and run the mixed C/assembly program.
2. Stop before calling the assembly helper.
3. Record `ebx`, `esp`, `ebp`, and `eip`.
4. Step into the assembly helper.
5. Inspect instructions with `disassemble` or `x/i $eip`.
6. Continue or step until return.
7. Compare `ebx` before and after the call.
8. Explain why callee-saved registers must be restored.

Terminology:

- `assembly helper / 汇编辅助函数`
- `callee-saved register / 被调用者保存寄存器`
- `caller-saved register / 调用者保存寄存器`
- `instruction / 指令`
- `step instruction / 单步执行指令`

ECE391 connection: Kernel and low-level coursework often crosses between C and assembly for entry stubs, context switching, interrupt handling, and hardware-facing code. Register preservation bugs can look unrelated to the assembly function unless the learner checks the calling convention.

## Supporting Repository Updates

Week 3 implementation should update the surrounding materials, not just add lab folders.

### Week Overview

Update `week3-build-asm-32bit/README.md` to include:

- Week 3 goal.
- Lab list with one-sentence descriptions.
- Required 32-bit package note.
- Verification command.
- Short reminder that Week 3 focuses on build artifacts and machine state.

### Verification Script

Add:

```text
scripts/verify-week3.sh
```

The script should:

- Run `make -C ... clean all` for each Week 3 lab.
- Exercise any non-destructive broken-build demo targets only if they are expected to fail and the script can clearly check that failure.
- Print a clear note if `-m32` support is missing.
- Avoid running long or interactive GDB sessions.

### Setup Docs

Update setup docs to mention 32-bit compilation support:

```bash
sudo apt install gcc-multilib libc6-dev-i386
```

If additional packages are needed for `objdump`, `readelf`, or `nm`, point learners to `binutils`.

### Command Cheatsheet

Add symptom-oriented recipes to `docs/command-cheatsheet.md`.

Suggested entries:

```text
Binary does not match source / 二进制和源码不一致:
  make -n
  make clean
  ls -l build/*.o build/program

Linker cannot find a function / 链接器找不到函数:
  nm build/*.o
  objdump -t build/*.o
  readelf -s build/*.o

Source view is not enough / 源码视角不够:
  disassemble
  info registers
  x/16xw $esp
  si
  ni

C calls assembly and state changes / C 调用汇编后状态变了:
  disassemble function
  info registers
  x/i $eip
  stepi
```

### Terminology Reference

Update `docs/terminology.md` with concise Chinese-English entries for:

- `target / 目标`
- `dependency / 依赖`
- `object file / 目标文件`
- `symbol / 符号`
- `linker / 链接器`
- `calling convention / 调用约定`
- `callee-saved register / 被调用者保存寄存器`
- `caller-saved register / 调用者保存寄存器`
- `instruction pointer / 指令指针`

## Error Handling and Environment Constraints

Week 3 has a higher chance of environment-specific failure than Week 1 or Week 2 because `-m32` requires multilib packages on a 64-bit Ubuntu VM.

Design requirements:

- Lab README files must distinguish project bugs from environment setup failures.
- The `Makefile` output should preserve compiler/linker errors rather than hiding them.
- The Week 3 overview should point to setup docs before the first 32-bit lab.
- `verify-week3.sh` should print a human-readable reminder when a 32-bit compile fails.

Expected learner-facing message:

```text
If -m32 fails, install 32-bit build support:
sudo apt install gcc-multilib libc6-dev-i386
```

## Testing Strategy

The implementation should be checked at three levels:

1. Build smoke tests:

   ```bash
   bash scripts/verify-week3.sh
   ```

2. Per-lab manual run:

   ```bash
   make -C week3-build-asm-32bit/lab07-makefile-deps run
   make -C week3-build-asm-32bit/lab09-x86-calling-convention run
   ```

3. Documentation walkthrough sanity check:

   - Every command in Guided Mode should be executable in the lab directory.
   - Broken-build demonstrations should be explicitly labeled and reproducible.
   - Review question answers should match the final code.

The verification script should not require interactive GDB. GDB commands remain learner exercises documented in the README files.

## Scope Boundaries

Week 3 will not:

- Introduce QEMU remote debugging.
- Require a bootloader or kernel image.
- Teach paging, interrupts, or exception handling beyond brief ECE391 motivation.
- Depend on nondeterministic crashes.
- Make the learner fix large Makefiles or real ECE391 code.

Those topics belong in Week 4 or in later project-specific practice.

## Success Criteria

Week 3 succeeds if the learner can:

- Recognize stale build symptoms and inspect Makefile dependency behavior.
- Use `nm`, `objdump -t`, and `readelf -s` to answer whether a symbol exists and where.
- Explain why GDB can or cannot break on a function name.
- Compile and debug a simple 32-bit x86 program.
- Connect C function arguments and return values to `esp`, `ebp`, stack words, and `eax`.
- Step through assembly instructions with `si`, `ni`, `stepi`, `nexti`, `x/i $eip`, and `disassemble`.
- Recognize a simple callee-saved register violation in a C/assembly call.
- Transfer these habits to ECE391-style build, linking, and low-level debugging problems.

## Recommended Implementation Order

1. Add `lab07-makefile-deps` because it teaches the build-artifact mindset needed by the rest of the week.
2. Add `lab08-linker-symbols` because symbol visibility sits between build systems and GDB.
3. Add 32-bit setup notes before adding labs that require `-m32`.
4. Add `lab09-x86-calling-convention`.
5. Add `lab10-c-and-asm`.
6. Add or update `scripts/verify-week3.sh`.
7. Update `week3-build-asm-32bit/README.md`, cheatsheet, and terminology docs.

This order gives the implementation a stable path: first build correctness, then symbol correctness, then 32-bit runtime inspection, then mixed-language debugging.
